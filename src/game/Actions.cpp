#include "game/Actions.hpp"

#include "main/Globals.hpp"

#include "game/Fighter.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/misc/Parsing.hpp>

namespace maths = sq::maths;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

const char* const FALLBACK_SCRIPT = R"lua(
  function tick()
    action:wait_until(32)
    action:finish_action()
  end
  function cancel() end
)lua";

const char* const VALIDATE_SCRIPT = R"lua(
  assert(type(tick) == 'function')
  assert(type(cancel) == 'function')
)lua";

//============================================================================//

Action::Action(FightWorld& world, Fighter& fighter, ActionType type, String path)
    : world(world), fighter(fighter), type(type), path(path)
    , mBlobs(world.get_hit_blob_allocator())
    , mEmitters(world.get_emitter_allocator()) {}

Action::~Action() = default;

//============================================================================//

void Action::do_start()
{
    reset_lua_thread();

    mTickCoroutine = sol::coroutine(mThread.state(), mTickFunction);
    sol::set_environment(mEnvironment, mTickCoroutine);

    SQASSERT(mTickCoroutine.valid(), "error creating coroutine");

    mCurrentFrame = 0u;
    mWaitingUntil = 0u;

    mFinished = false;
}

//----------------------------------------------------------------------------//

bool Action::do_tick()
{
    // todo: this doesn't need to happen every frame
    for (auto& [key, emitter] : mEmitters)
    {
        Mat4F matrix = fighter.get_model_matrix();

        if (emitter.bone >= 0)
        {
            const auto& matrices = fighter.get_bone_matrices();
            const auto& boneMatrix = matrices[uint(emitter.bone)];
            matrix *= maths::transpose(Mat4F(boneMatrix));
        }

        emitter.emitPosition = Vec3F(matrix * Vec4F(emitter.origin, 1.f));
        emitter.emitVelocity = Vec3F(fighter.get_velocity().x * 0.2f, 0.f, 0.f);
    }

    if (world.globals.editorMode == false)
        SQASSERT(mTickCoroutine.runnable(), "finish_action not called");

    if (mCurrentFrame >= mWaitingUntil)
    {
        const auto pfr = mTickCoroutine.call();
        if (pfr.valid() == false)
        {
            sq::log_warning("caught script error, will crash now");
            sol::script_throw_on_error(mThread.state(), pfr);
        }
    }

    mCurrentFrame += 1u;

    return mFinished;
}

//----------------------------------------------------------------------------//

void Action::do_cancel()
{
    const auto pfr = mCancelFunction.call();

    if (pfr.valid() == false)
    {
        sq::log_warning("caught script error, will crash now");
        sol::script_throw_on_error(mThread.state(), pfr);
    }

    world.disable_all_hit_blobs(fighter);
    world.reset_all_hit_blob_groups(fighter);
}

//============================================================================//

void Action::load_from_json()
{
    mBlobs.clear();
    mEmitters.clear();

    const String filePath = path + ".json";

    if (sq::check_file_exists(filePath) == false)
    {
        sq::log_warning("missing json   '%s'", filePath);
        return;
    }

    const JsonValue root = sq::parse_json_from_file(filePath);

    Vector<String> errors;

    try {
    for (auto iter : root.at("blobs").items())
    {
        HitBlob& blob = mBlobs[iter.key()];

        blob.fighter = &fighter;
        blob.action = this;

        try { blob.from_json(iter.value()); }
        catch (const std::exception& e) {
            errors.push_back("blob '%s': %s"_fmt_(iter.key(), e.what()));
        }
    }
    } catch (const std::exception& e) { errors.emplace_back(e.what()); }

    try {
    for (auto iter : root.at("emitters").items())
    {
        ParticleEmitter& emitter = mEmitters[iter.key()];

        emitter.fighter = &fighter;
        emitter.action = this;

        try { emitter.from_json(iter.value()); }
        catch (const std::exception& e) {
            errors.push_back("emitter '%s': %s"_fmt_(iter.key(), e.what()));
        }
    }
    } catch (const std::exception& e) { errors.emplace_back(e.what()); }

    if (errors.empty() == false)
    {
        sq::log_warning_block("errors in json '%s'"_fmt_(filePath), errors);
        mBlobs.clear();
        mEmitters.clear();
    }
}

//----------------------------------------------------------------------------//

void Action::load_lua_from_file()
{
    // todo: This function has to load the script file twice, once to set mLuaSource
    // and again to load the script. Need to figure out how to have lua load a script
    // from a string, but still give error messages as if it was loading a file.

    const String filePath = path + ".lua";

    if (sq::check_file_exists(filePath) == false)
    {
        sq::log_warning("missing script '%s'", filePath);
        load_lua_from_string(FALLBACK_SCRIPT);
        return;
    }

    mLuaSource = sq::get_string_from_file(filePath);

    try
    {
        reset_lua_environment();

        world.lua.script_file(filePath, mEnvironment);
        world.lua.script(VALIDATE_SCRIPT, mEnvironment, "validate", sol::load_mode::any);

        mTickFunction = mEnvironment["tick"];
        mCancelFunction = mEnvironment["cancel"];
    }
    catch (sol::error& error)
    {
        sq::log_warning("error loading script '%s'\n%s", filePath, error.what());
        load_lua_from_string(FALLBACK_SCRIPT);
    }
}

//----------------------------------------------------------------------------//

void Action::load_lua_from_string(StringView source)
{
    try
    {
        reset_lua_environment();

        world.lua.script(source, mEnvironment, "source", sol::load_mode::any);
        world.lua.script(VALIDATE_SCRIPT, mEnvironment, "validate", sol::load_mode::any);

        mTickFunction = mEnvironment["tick"];
        mCancelFunction = mEnvironment["cancel"];
    }
    catch (sol::error& error)
    {
        sq::log_warning("error loading script '%s.lua'", path);
        sq::log_only(error.what());

        if (source.data() == FALLBACK_SCRIPT)
            sq::log_error("fallback script is broken???");

        load_lua_from_string(FALLBACK_SCRIPT);
    }
}

//============================================================================//

void Action::reset_lua_environment()
{
    mEnvironment = { world.lua, sol::create, world.lua.globals() };

    mEnvironment["action"] = this;
    mEnvironment["fighter"] = &fighter;
}

void Action::reset_lua_thread()
{
    if (mThread.valid() == true)
    {
        lua_gc(mThread.state(), LUA_GCCOLLECT);
        lua_resetthread(mThread.state());
    }
    else mThread = sol::thread::create(world.lua);
}

//============================================================================//

bool Action::has_changes(const Action& reference) const
{
    if (mBlobs != reference.mBlobs) return true;
    if (mEmitters != reference.mEmitters) return true;
    if (mLuaSource != reference.mLuaSource) return true;
    return false;
}

void Action::apply_changes(const Action& source)
{
    mBlobs.clear();
    for (const auto& [key, blob] : source.mBlobs)
        mBlobs.try_emplace(key, blob);

    mEmitters.clear();
    for (const auto& [key, emitter] : source.mEmitters)
        mEmitters.try_emplace(key, emitter);

    mLuaSource = source.mLuaSource;
    load_lua_from_string(mLuaSource);
}

UniquePtr<Action> Action::clone() const
{
    auto result = std::make_unique<Action>(world, fighter, type, path);

    result->apply_changes(*this);

    return result;
}

//============================================================================//

void Action::lua_func_enable_blob(TinyString key)
{
    sq::log_only("enable_blob('%s')", key);

    const auto iter = mBlobs.find(key);
    if (iter == mBlobs.end())
        throw "invalid blob '%s'"_fmt_(key);

    world.enable_hit_blob(&iter->second);
};

//----------------------------------------------------------------------------//

void Action::lua_func_disable_blob(TinyString key)
{
    sq::log_only("disable_blob('%s')", key);

    const auto iter = mBlobs.find(key);
    if (iter == mBlobs.end())
        throw "invalid blob '%s'"_fmt_(key);

    world.disable_hit_blob(&iter->second);
};

//----------------------------------------------------------------------------//

void Action::lua_func_finish_action()
{
    sq::log_only("finish_action()");

    mFinished = true;
};

//----------------------------------------------------------------------------//

void Action::lua_func_emit_particles(TinyString key, uint count)
{
    sq::log_only("emit_particles('%s', %d)", key, count);

    const auto iter = mEmitters.find(key);
    if (iter == mEmitters.end())
        throw "invalid emitter '%s'"_fmt_(key);

    iter->second.generate(world.get_particle_system(), count);
}

//----------------------------------------------------------------------------//

void Action::lua_func_wait_until(uint frame)
{
    sq::log_only("wait_until(%d)", frame);

    if (frame <= mCurrentFrame)
        throw "can't wait for %d on %d"_fmt_(frame, mCurrentFrame);

    mWaitingUntil = frame;
}
