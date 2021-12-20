#include "game/FighterAction.hpp"

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

constexpr char FALLBACK_SOURCE[] = R"wren(
import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  execute() {
    return vars.onGround ? "Dodge" : "AirDodge"
  }
}
)wren";

//============================================================================//

FighterAction::FighterAction(Fighter& fighter, SmallString name)
    : fighter(fighter), world(fighter.world), name(name)
{
}

FighterAction::~FighterAction()
{
    if (mScriptHandle) wrenReleaseHandle(world.vm, mScriptHandle);
    if (mFiberHandle) wrenReleaseHandle(world.vm, mFiberHandle);
}

//============================================================================//

void FighterAction::call_do_start()
{
    SQASSERT(fighter.activeAction == this, "action not active");

    const auto error = world.vm.safe_call_void(world.handles.action_do_start, this);
    if (error.empty() == false)
        set_error_message("call_do_start", error);
}

//============================================================================//

void FighterAction::call_do_updates()
{
    SQASSERT(fighter.activeAction == this, "action not active");

    const auto error = world.vm.safe_call_void(world.handles.action_do_updates, this);
    if (error.empty() == false)
        set_error_message("call_do_updates", error);
}

//============================================================================//

void FighterAction::call_do_cancel()
{
    SQASSERT(fighter.activeAction == this, "action not active");

    const auto error = world.vm.safe_call_void(world.handles.action_do_cancel, this);
    if (error.empty() == false)
        set_error_message("call_do_cancel", error);
}

//============================================================================//

void FighterAction::load_json_from_file()
{
    mBlobs.clear();
    mEffects.clear();
    mEmitters.clear();
    mSounds.clear();

    const String path = "assets/fighters/{}/actions/{}.json"_format(fighter.name, name);

    const auto root = sq::try_parse_json_from_file(path);
    if (root.has_value() == false)
    {
        sq::log_warning("missing json   '{}'", path);
        return;
    }

    String errors;

    TRY_FOR (const auto& item : root->at("blobs").items())
    {
        HitBlob& blob = mBlobs[item.key()];
        blob.action = this;

        try { blob.from_json(item.value()); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\nblob '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    TRY_FOR (const auto& item : root->at("effects").items())
    {
        VisualEffect& effect = mEffects[item.key()];
        effect.cache = &world.caches.effects;
        effect.fighter = &fighter;

        try { effect.from_json(item.value()); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\neffect '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    TRY_FOR (const auto& item : root->at("emitters").items())
    {
        Emitter& emitter = mEmitters[item.key()];
        emitter.fighter = &fighter;

        try { emitter.from_json(item.value()); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\nemitter '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    TRY_FOR (const auto& item : root->at("sounds").items())
    {
        SoundEffect& sound = mSounds[item.key()];
        sound.cache = &world.caches.sounds;

        try { sound.from_json(item.value()); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\nsound '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    if (errors.empty() == false)
        sq::log_warning_multiline("errors in '{}'{}", path, errors);
}

//============================================================================//

void FighterAction::load_wren_from_file()
{
    const String path = "assets/fighters/{}/actions/{}.wren"_format(fighter.name, name);

    // set mWrenSource to either the file contents or a fallback script
    auto source = sq::try_read_text_from_file(path);
    if (source.has_value() == false)
    {
        sq::log_warning("missing script '{}'", path);
        source = sq::try_read_text_from_file("wren/actions/{}.wren"_format(name));
        if (source.has_value() == false) source = FALLBACK_SOURCE;
    }
    mWrenSource = std::move(*source);

    // parse mWrenSource and construct wren Script object
    load_wren_from_string();
}

//============================================================================//

void FighterAction::load_wren_from_string()
{
    // todo:
    // - mWrenSource should be part of the editor
    // - fighters should be able to share identical modules

    const String module = "{}_{}"_format(fighter.index, name);

    // if the script is already loaded, unload it
    if (mScriptHandle != nullptr)
    {
        // editor crashes sometimes, probably because of my hacky module unloading
        wrenReleaseHandle(world.vm, mScriptHandle);
        wrenUnloadModule(world.vm, module.c_str());
        fighter.clear_error_message(this);
    }

    // interpret wren source string into a new module
    const auto error = world.vm.safe_interpret(module.c_str(), mWrenSource.c_str());
    if (error.empty() == false)
    {
        set_error_message("load_wren_from_string", error);
        wrenUnloadModule(world.vm, module.c_str());

        auto source = sq::try_read_text_from_file("wren/actions/{}.wren"_format(name));
        if (source.has_value() == false) source = FALLBACK_SOURCE;

        world.vm.interpret(module.c_str(), source->c_str());
    }

    // create a new instance of the Script object
    const auto safe = world.vm.safe_call<WrenHandle*>(world.handles.new_1, wren::GetVar(module.c_str(), "Script"), this);
    if (safe.ok == false)
    {
        set_error_message("load_wren_from_string", error);
        wrenUnloadModule(world.vm, module.c_str());

        auto source = sq::try_read_text_from_file("wren/actions/{}.wren"_format(name));
        if (source.has_value() == false) source = FALLBACK_SOURCE;

        world.vm.interpret(module.c_str(), source->c_str());
        mScriptHandle = world.vm.call<WrenHandle*>(world.handles.new_1, wren::GetVar(module.c_str(), "Script"), this);
    }
    else mScriptHandle = safe.value;

    // don't need the string anymore, so free some memory
    if (world.options.editor_mode == false)
    {
        mWrenSource.clear();
        mWrenSource.shrink_to_fit();
    }
}

//============================================================================//

bool FighterAction::has_changes(const FighterAction& reference) const
{
    if (mBlobs != reference.mBlobs) return true;
    if (mEffects != reference.mEffects) return true;
    if (mEmitters != reference.mEmitters) return true;
    if (mSounds != reference.mSounds) return true;
    if (mWrenSource != reference.mWrenSource) return true;
    return false;
}

void FighterAction::apply_changes(const FighterAction& source)
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

std::unique_ptr<FighterAction> FighterAction::clone() const
{
    auto result = std::make_unique<FighterAction>(fighter, name);

    result->apply_changes(*this);

    return result;
}

//============================================================================//

void FighterAction::set_error_message(StringView method, StringView error)
{
    String message = "Action '{}/{}'\n{}\nC++ | {}()\n"_format(fighter.name, name, error, method);

    if (world.options.editor_mode == false)
        sq::log_error_multiline(message);

    fighter.set_error_message(this, std::move(message));
}
