#include "game/Action.hpp"

#include "main/Options.hpp"

#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/misc/Parsing.hpp>

using namespace sts;

//============================================================================//

const char* const FALLBACK_SCRIPT = R"lua(
function tick()
  action:wait_until(32)
  action:allow_interrupt()
end
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
    if (world.options.log_script == true)
        sq::log_debug("{}/{}: do_start()", fighter.type, type);

    reset_lua_thread();

    mTickCoroutine = sol::coroutine(mThread.state(), mTickFunction);
    sol::set_environment(mEnvironment, mTickCoroutine);

    SQASSERT(mTickCoroutine.valid(), "error creating coroutine");

    mCurrentFrame = 0u;
    mWaitingUntil = 0u;

    mAllowIterrupt = false;
}

//----------------------------------------------------------------------------//

void Action::do_tick()
{
    mCurrentFrame += 1u;

    if (mTickCoroutine.status() == sol::call_status::ok)
    {
        mStatus = ActionStatus::Finished;
        return;
    }

    if (mTickCoroutine.status() != sol::call_status::yielded)
    {
        mStatus = ActionStatus::RuntimeError;
        return;
    }

    SQASSERT(mTickCoroutine.runnable() == true, "");

    if (mCurrentFrame >= mWaitingUntil)
    {
        const auto pfr = mTickCoroutine.call();
        if (pfr.valid() == false)
        {
            try { sol::script_throw_on_error(mThread.state(), pfr); }
            catch (const sol::error& error)
            {
                if (world.options.editor_mode == false)
                    sq::log_error_multiline("action '{}/{}', frame {}:\n{}", fighter.type, type, mCurrentFrame, error.what());

                mErrorMessage = "error on frame {}:\n{}"_format(mCurrentFrame, error.what());
                do_cancel();
            }
        }
    }

    mStatus = mAllowIterrupt ? ActionStatus::AllowInterrupt : ActionStatus::Running;
}

//----------------------------------------------------------------------------//

void Action::do_cancel()
{
    if (world.options.log_script == true)
        sq::log_debug("{}/{}: do_cancel()", fighter.type, type);

    world.disable_all_hit_blobs(fighter);
    world.reset_all_hit_blob_groups(fighter);
    mAllowIterrupt = true;

    //const auto pfr = mCancelFunction.call();
//    if (pfr.valid() == false)
//    {
//        sq::log_warning("caught script error, will crash now");
//        sol::script_throw_on_error(mThread.state(), pfr);
//    }

//    world.reset_all_hit_blob_groups(fighter);
//    world.disable_all_hit_blobs(fighter);
}

//============================================================================//

void Action::load_from_json()
{
    mBlobs.clear();
    mEmitters.clear();

    const String filePath = path + ".json";

    if (sq::check_file_exists(filePath) == false)
    {
        sq::log_warning("missing json   '{}'", filePath);
        return;
    }

    const JsonValue root = sq::parse_json_from_file(filePath);

    String errors;

    TRY_FOR (auto iter : root.at("blobs").items())
    {
        HitBlob& blob = mBlobs[iter.key()];

        blob.fighter = &fighter;
        blob.action = this;

        try { blob.from_json(iter.value()); }
        catch (const std::exception& e) { errors += "\nblob '{}': {}"_format(iter.key(), e.what()); }
    }
    CATCH (const std::exception& e) { errors += '\n'; errors += e.what(); }

    TRY_FOR (auto iter : root.at("emitters").items())
    {
        Emitter& emitter = mEmitters[iter.key()];

        emitter.fighter = &fighter;
        emitter.action = this;

        try { emitter.from_json(iter.value()); }
        catch (const std::exception& e) { errors += "\nemitter '{}': {}"_format(iter.key(), e.what()); }
    }
    CATCH (const std::exception& e) { errors += '\n'; errors += e.what(); }

    if (errors.empty() == false)
    {
        sq::log_warning_multiline("errors in json '{}'{}", filePath, errors);
        mBlobs.clear();
        mEmitters.clear();
    }
}

//----------------------------------------------------------------------------//

void Action::load_lua_from_fallback()
{
    sol::state& lua = world.get_lua_state();

    reset_lua_environment();

    lua.script(FALLBACK_SCRIPT, mEnvironment, "fallback", sol::load_mode::text);

    mTickFunction = mEnvironment["tick"];
}

//----------------------------------------------------------------------------//

void Action::load_lua_from_file()
{
    const String filePath = path + ".lua";

    if (sq::check_file_exists(filePath) == false)
    {
        sq::log_warning("missing script '{}'", filePath);
        load_lua_from_fallback();

        if (world.options.editor_mode == true)
        {
            mLuaSource = FALLBACK_SCRIPT;
            mErrorMessage.clear();
        }
    }

    else if (world.options.editor_mode == true)
    {
        mLuaSource = sq::get_string_from_file(filePath);
        load_lua_from_string();
    }

    else try
    {
        sol::state& lua = world.get_lua_state();
        reset_lua_environment();

        lua.script_file(filePath, mEnvironment, sol::load_mode::text);

        mTickFunction = mEnvironment["tick"];
    }
    catch (const sol::error& error)
    {
        sq::log_warning_multiline("failed to load script '{}'\n{}", filePath, error.what());
        load_lua_from_fallback();
    }
}

//----------------------------------------------------------------------------//

void Action::load_lua_from_string()
{
    try
    {
        sol::state& lua = world.get_lua_state();
        reset_lua_environment();

        lua.script(mLuaSource, mEnvironment, "src", sol::load_mode::text);

        mTickFunction = mEnvironment["tick"];

        mErrorMessage.clear();
    }
    catch (const sol::error& error)
    {
        load_lua_from_fallback();
        mErrorMessage = "error loading from string:\n{}"_format(error.what());
    }

    //reset_lua_thread();
}

//============================================================================//

void Action::reset_lua_environment()
{
    sol::state& lua = world.get_lua_state();

    mEnvironment = { lua, sol::create, lua.globals() };

    mEnvironment["action"] = this;
    mEnvironment["fighter"] = &fighter;
}

void Action::reset_lua_thread()
{
    sol::state& lua = world.get_lua_state();

    if (mThread.valid() == true)
    {
        //lua_gc(mThread.state(), LUA_GCCOLLECT);
        lua_resetthread(mThread.state());
    }
    else mThread = sol::thread::create(lua);
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
    load_lua_from_string();
}

std::unique_ptr<Action> Action::clone() const
{
    auto result = std::make_unique<Action>(world, fighter, type, path);

    result->apply_changes(*this);

    return result;
}

//============================================================================//

template <class... Args>
static inline void throw_error(StringView str, const Args&... args)
{
    throw std::runtime_error(fmt::format(str, args...));
}

template <class... Args>
inline void Action::log_script(StringView str, const Args&... args)
{
    if (world.options.log_script == true)
        sq::log_debug(sq::build_string("{}/{} {:02d}: ", str), fighter.type, type, mCurrentFrame, args...);
}

//============================================================================//

//void Action::lua_func_enable_blob(TinyString key)
//{
//    if (world.options.log_script) sq::log_debug("enable_blob('{}')", key);

//    const auto iter = mBlobs.find(key);
//    if (iter == mBlobs.end())
//        throw_error("invalid blob '{}'", key);

//    world.enable_hit_blob(&iter->second);
//};

//----------------------------------------------------------------------------//

//void Action::lua_func_disable_blob(TinyString key)
//{
//    if (world.options.log_script) sq::log_debug("disable_blob('{}')", key);

//    const auto iter = mBlobs.find(key);
//    if (iter == mBlobs.end())
//        throw_error("invalid blob '{}'", key);

//    world.disable_hit_blob(&iter->second);
//};

//----------------------------------------------------------------------------//

void Action::lua_func_enable_blob_group(uint8_t group)
{
    log_script("enable_blob_group('{}')", group);

    for (auto& [key, blob] : mBlobs)
        if (blob.group == group)
            world.enable_hit_blob(&blob);
};

//----------------------------------------------------------------------------//

void Action::lua_func_disable_blob_group(uint8_t group)
{
    log_script("disable_blob_group('{}')", group);

    for (auto& [key, blob] : mBlobs)
        if (blob.group == group)
            world.disable_hit_blob(&blob);
};

//----------------------------------------------------------------------------//

void Action::lua_func_allow_interrupt()
{
    log_script("allow_interrupt()");

    world.disable_all_hit_blobs(fighter);
    world.reset_all_hit_blob_groups(fighter);
    mAllowIterrupt = true;
};

//----------------------------------------------------------------------------//

void Action::lua_func_emit_particles(TinyString key, uint count)
{
    log_script("emit_particles('{}', {})", key, count);

    if (count > 256u)
        throw_error("count {} too high, limit is 256", count);

    const auto iter = mEmitters.find(key);
    if (iter == mEmitters.end())
        throw_error("invalid emitter '{}'", key);

    Emitter& emitter = iter->second;

    emitter.generate(world.get_particle_system(), count);
}

//----------------------------------------------------------------------------//

void Action::lua_func_wait_until(uint frame)
{
    log_script("wait_until({})", frame);

    if (frame <= mCurrentFrame)
        throw_error("can't wait for {} on {}", frame, mCurrentFrame);

    mWaitingUntil = frame;
}
