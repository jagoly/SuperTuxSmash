#include "game/Action.hpp"

#include "main/Options.hpp"

#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

const char* const FALLBACK_SCRIPT
= R"wren(

import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(32)
    action.allow_interrupt()
  }

  cancel() {
    //fighter.set_intangible(false)
    //action.disable_hitblobs()
  }
}

)wren";

//============================================================================//

Action::Action(FightWorld& world, Fighter& fighter, ActionType type, String path)
    : type(type), fighter(fighter), world(world), path(path)
//    , mBlobs(world.get_hit_blob_allocator())
//    , mEmitters(world.get_emitter_allocator())
//    , mSounds(world.get_sound_effect_allocator())
    , mBlobs(world.get_memory_resource())
    , mEmitters(world.get_memory_resource())
    , mSounds(world.get_memory_resource())
{
    load_from_json();
    load_wren_from_file();
}

Action::~Action()
{
    if (mScriptInstance) wrenReleaseHandle(world.vm, mScriptInstance);
    if (mFiberInstance) wrenReleaseHandle(world.vm, mFiberInstance);
}

//============================================================================//

void Action::do_start()
{
    if (world.options.log_script == true)
        sq::log_debug("{}/{}     | do_start()", fighter.type, type);

    if (mFiberInstance) wrenReleaseHandle(world.vm, mFiberInstance);

    mFiberInstance = world.vm.call<WrenHandle*>(world.handles.script_reset, mScriptInstance);

    mCurrentFrame = 0u;
    mWaitingUntil = 0u;

    mStatus = ActionStatus::Running;
}

//============================================================================//

void Action::do_tick()
//void Action::do_tick(const InputFrame& input)
{
    SQASSERT(mStatus != ActionStatus::None, "do_tick() called on unstarted action");
    SQASSERT(mStatus != ActionStatus::Finished, "do_tick() called on finished action");

    if (world.vm.call<bool>(world.handles.fiber_isDone, mFiberInstance))
    {
        if (mStatus == ActionStatus::RuntimeError)
        {
            world.disable_hitblobs(*this);
            world.reset_all_collisions(fighter.index);
            mStatus = ActionStatus::Finished;
        }

        else if (mStatus == ActionStatus::AllowInterrupt)
        {
            mStatus = ActionStatus::Finished;
        }

        else if (mStatus == ActionStatus::Running)
            set_error_status("frame {}:\nexecute() returned before calling allow_interrupt()", mCurrentFrame);
    }

    else
    {
        if (mCurrentFrame == mWaitingUntil)
        {
            const auto errors = world.vm.safe_call_void(world.handles.fiber_call, mFiberInstance);

            if (errors.has_value() == true)
                set_error_status("frame {}:\n{}", mCurrentFrame, *errors);
        }

        mCurrentFrame += 1u;
    }

    // status == Running:        the coroutine is running or just finished, allow_interrupt has not been called
    // status == AllowInterrupt: the coroutine is running or just finished, allow_interrupt has been called
    // status == Finished:       the coroutine finished last frame, the action should be deactivated
    // status == RuntimeError:   some kind of error has occured while in editor mode
}

//============================================================================//

void Action::do_cancel()
{
    if (world.options.log_script == true)
        sq::log_debug("{}/{}     | do_cancel()", fighter.type, type);

    const auto errors = world.vm.safe_call_void(world.handles.script_cancel, mScriptInstance);

    if (errors.has_value() == true)
        set_error_status("cancel():\n{}", *errors);

    // even if cancel() raised an error, always set status to finished
    mStatus = ActionStatus::Finished;
}

//============================================================================//

void Action::load_from_json()
{
    mBlobs.clear();
    mEmitters.clear();

    // todo: make sqee better so we don't have use check_file_exists

    if (sq::check_file_exists(path + ".json") == false)
    {
        sq::log_warning("missing json   '{}.json'", path);
        return;
    }

    const JsonValue root = sq::parse_json_from_file(path + ".json");

    String errors;

    TRY_FOR (auto iter : root.at("blobs").items())
    {
        HitBlob& blob = mBlobs[iter.key()];
        blob.action = this;

        try { blob.from_json(iter.value()); }
        catch (const std::exception& e) { errors += "\nblob '{}': {}"_format(iter.key(), e.what()); }
    }
    CATCH (const std::exception& e) { errors += '\n'; errors += e.what(); }

    TRY_FOR (auto iter : root.at("emitters").items())
    {
        Emitter& emitter = mEmitters[iter.key()];
        emitter.fighter = &fighter;

        try { emitter.from_json(iter.value()); }
        catch (const std::exception& e) { errors += "\nemitter '{}': {}"_format(iter.key(), e.what()); }
    }
    CATCH (const std::exception& e) { errors += '\n'; errors += e.what(); }

    TRY_FOR (auto iter : root.at("sounds").items())
    {
        SoundEffect& sound = mSounds[iter.key()];
        sound.cache = &world.sounds;

        try { sound.from_json(iter.value()); }
        catch (const std::exception& e) { errors += "\nsound '{}': {}"_format(iter.key(), e.what()); }
    }
    CATCH (const std::exception& e) { errors += '\n'; errors += e.what(); }

    if (errors.empty() == false)
        sq::log_warning_multiline("errors in '{}.json'{}", path, errors);
}

//============================================================================//

void Action::load_wren_from_file()
{
    // Called to load the script from a file. Real work is done load_wren_from_string().

    auto source = sq::try_get_string_from_file(path + ".wren");

    if (source.has_value() == false)
    {
        sq::log_warning("missing script '{}.wren'", path);
        mWrenSource = FALLBACK_SCRIPT;
    }
    else mWrenSource = std::move(*source);

    load_wren_from_string();
}

//----------------------------------------------------------------------------//

void Action::load_wren_from_string()
{
    // Load and construct wren script instance. In editor mode, will load fallback on failure.

    const String module = "{}_{}"_format(fighter.index, type);

    // if the script is already loaded, unload it
    if (mScriptInstance != nullptr)
    {
        wrenReleaseHandle(world.vm, mScriptInstance);
        wrenUnloadModule(world.vm, module.c_str());
        mErrorMessage.clear();
    }

    auto errors = world.vm.safe_interpret(module.c_str(), mWrenSource.c_str());
    if (errors.has_value() == true)
    {
        if (world.options.editor_mode) mErrorMessage = std::move(*errors);
        else sq::log_error_multiline("failed to load script '{}.wren'\n{}", path, *errors);

        wrenUnloadModule(world.vm, module.c_str());
        world.vm.interpret(module.c_str(), FALLBACK_SCRIPT);
    }

    const auto safe = world.vm.safe_call<WrenHandle*>(world.handles.script_new, wren::GetVar{module.c_str(), "Script"}, this, &fighter);
    if (safe.errors.empty() == false)
    {
        if (world.options.editor_mode) mErrorMessage = std::move(safe.errors);
        else sq::log_error_multiline("failed to load script '{}.wren'\n{}", path, safe.errors);

        wrenUnloadModule(world.vm, module.c_str());
        world.vm.interpret(module.c_str(), FALLBACK_SCRIPT);
        mScriptInstance = world.vm.call<WrenHandle*>(world.handles.script_new, wren::GetVar{module.c_str(), "Script"}, this, &fighter);
    }
    else mScriptInstance = *safe.value;

    // don't need the source anymore, so free some memory
    if (world.options.editor_mode == false)
    {
        mWrenSource.clear();
        mWrenSource.shrink_to_fit();
    }
}

//============================================================================//

bool Action::has_changes(const Action& reference) const
{
    if (mBlobs != reference.mBlobs) return true;
    if (mEmitters != reference.mEmitters) return true;
    if (mSounds != reference.mSounds) return true;
    if (mWrenSource != reference.mWrenSource) return true;
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

    mSounds.clear();
    for (const auto& [key, sound] : source.mSounds)
        mSounds.try_emplace(key, sound);

    mWrenSource = source.mWrenSource;
    load_wren_from_string();
}

std::unique_ptr<Action> Action::clone() const
{
    auto result = std::make_unique<Action>(world, fighter, type, path);

    result->apply_changes(*this);

    return result;
}

//============================================================================//

template <class... Args>
inline void Action::set_error_status(StringView str, const Args&... args)
{
    if (world.options.editor_mode == false)
        sq::log_error_multiline(sq::build_string("action '{}/{}' - ", str), fighter.type, type, args...);

    mErrorMessage = fmt::format(str, args...);
    mStatus = ActionStatus::RuntimeError;
}
