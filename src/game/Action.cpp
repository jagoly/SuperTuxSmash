#include "game/Action.hpp"

#include "main/Options.hpp"

#include "game/Emitter.hpp"
#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"
#include "game/HitBlob.hpp"
#include "game/SoundEffect.hpp"
#include "game/VisualEffect.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

static const auto FALLBACK_SCRIPT
= R"wren(import "sts" for ScriptBase

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
})wren";

//============================================================================//

Action::Action(Fighter& fighter, ActionType type)
    : fighter(fighter), world(fighter.world), type(type)
{
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

    // in brawl, this effectively gets done at the end of attack scripts
    // for now at least, it seems reasonable to just do it at the start of any action
    world.reset_collisions(fighter.index);
}

//============================================================================//

void Action::do_tick()
{
    SQASSERT(mStatus != ActionStatus::None, "do_tick() called on unstarted action");
    SQASSERT(mStatus != ActionStatus::Finished, "do_tick() called on finished action");

    if (world.vm.call<bool>(world.handles.fiber_isDone, mFiberInstance) == true)
    {
        if (mStatus == ActionStatus::AllowInterrupt)
        {
            // the action completed without any issues
            mStatus = ActionStatus::Finished;
        }

        else if (mStatus == ActionStatus::RuntimeError)
        {
            // finished with an error, do some clean up
            world.disable_hitblobs(*this);
            world.reset_collisions(fighter.index);
            mStatus = ActionStatus::Finished;
        }

        else if (mStatus == ActionStatus::Running)
        {
            if (ActionTraits::get(type).needInterrupt == true)
            {
                impl_set_error_message("execute():\nreturned before calling allow_interrupt()");
                mStatus = ActionStatus::RuntimeError;
            }
            else mStatus = ActionStatus::Finished;
        }
    }

    else
    {
        if (mCurrentFrame == mWaitingUntil)
        {
            const auto errors = world.vm.safe_call_void(world.handles.fiber_call, mFiberInstance);

            if (errors.has_value() == true)
            {
                impl_set_error_message("frame {}:\n{}", mCurrentFrame, *errors);
                mStatus = ActionStatus::RuntimeError;
            }
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
        impl_set_error_message("cancel():\n{}", *errors);

    // always set to finished, even if cancel raises an error
    mStatus = ActionStatus::Finished;
}

//============================================================================//

void Action::load_json_from_file()
{
    mBlobs.clear();
    mEmitters.clear();

    // todo: make sqee better so we don't have use check_file_exists

    const String path = build_path(".json");

    if (sq::check_file_exists(path) == false)
    {
        sq::log_warning("missing json   '{}'", path);
        return;
    }

    const JsonValue root = sq::parse_json_from_file(path);

    String errors;

    TRY_FOR (auto iter : root.at("blobs").items())
    {
        HitBlob& blob = mBlobs[iter.key()];
        blob.action = this;

        try { blob.from_json(iter.value()); }
        catch (const std::exception& e) { errors += "\nblob '{}': {}"_format(iter.key(), e.what()); }
    }
    CATCH (const std::exception& e) { errors += '\n'; errors += e.what(); }

    TRY_FOR (auto iter : root.at("effects").items())
    {
        VisualEffect& effect = mEffects[iter.key()];
        effect.cache = &world.caches.effects;
        effect.fighter = &fighter;

        try { effect.from_json(iter.value()); }
        catch (const std::exception& e) {
            errors += "\neffect '{}': {}"_format(iter.key(), e.what());
        }
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
        sound.cache = &world.caches.sounds;

        try { sound.from_json(iter.value()); }
        catch (const std::exception& e) { errors += "\nsound '{}': {}"_format(iter.key(), e.what()); }
    }
    CATCH (const std::exception& e) { errors += '\n'; errors += e.what(); }

    if (errors.empty() == false)
        sq::log_warning_multiline("errors in '{}'{}", path, errors);
}

//============================================================================//

void Action::load_wren_from_file()
{
    const String path = build_path(".wren");

    // set mWrenSource to either the file contents or the fallback script
    auto source = sq::try_get_string_from_file(path);
    if (source.has_value() == false)
    {
        sq::log_warning("missing script '{}'", path);
        mWrenSource = FALLBACK_SCRIPT;
    }
    else mWrenSource = std::move(*source);

    // parse mWrenSource and construct wren Script object
    load_wren_from_string();
}

//============================================================================//

void Action::load_wren_from_string()
{
    const String module = "{}_{}"_format(fighter.index, type);

    // if the script is already loaded, unload it
    if (mScriptInstance != nullptr)
    {
        // editor crashes sometimes, probably because of my hacky module unloading
        wrenReleaseHandle(world.vm, mScriptInstance);
        wrenUnloadModule(world.vm, module.c_str());
        mErrorMessage.clear();
    }

    // interpret wren source string into a new module
    const auto errors = world.vm.safe_interpret(module.c_str(), mWrenSource.c_str());
    if (errors.has_value() == true)
    {
        impl_set_error_message("failed to load wren script\n{}", *errors);
        wrenUnloadModule(world.vm, module.c_str());
        world.vm.interpret(module.c_str(), FALLBACK_SCRIPT);
    }

    // create a new instance of the Script object
    const auto safe = world.vm.safe_call<WrenHandle*>(world.handles.script_new, wren::GetVar{module.c_str(), "Script"}, this, &fighter);
    if (safe.errors.empty() == false)
    {
        impl_set_error_message("failed to load wren script\n{}", *errors);
        wrenUnloadModule(world.vm, module.c_str());
        world.vm.interpret(module.c_str(), FALLBACK_SCRIPT);
        mScriptInstance = world.vm.call<WrenHandle*>(world.handles.script_new, wren::GetVar{module.c_str(), "Script"}, this, &fighter);
    }
    else mScriptInstance = *safe.value;

    // don't need the string anymore, so free some memory
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
    if (mEffects != reference.mEffects) return true;
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

    mEffects.clear();
    for (const auto& [key, blob] : source.mEffects)
        mEffects.try_emplace(key, blob);

    mEmitters.clear();
    for (const auto& [key, emitter] : source.mEmitters)
        mEmitters.try_emplace(key, emitter);

    mSounds.clear();
    for (const auto& [key, sound] : source.mSounds)
        mSounds.try_emplace(key, sound);

    mWrenSource = source.mWrenSource;
}

std::unique_ptr<Action> Action::clone() const
{
    auto result = std::make_unique<Action>(fighter, type);

    result->apply_changes(*this);

    return result;
}

//============================================================================//

String Action::build_path(StringView extension) const
{
    return sq::build_string("assets/fighters/", sq::enum_to_string(fighter.type), "/actions/",
                            sq::enum_to_string(type), extension);
}

template <class... Args>
inline void Action::impl_set_error_message(StringView str, const Args&... args)
{
    mErrorMessage = fmt::format(str, args...);

    if (world.options.editor_mode == false)
        sq::log_error_multiline("action '{}/{}' - {}", fighter.type, type, mErrorMessage);
}
