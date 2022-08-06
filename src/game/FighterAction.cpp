#include "game/FighterAction.hpp"

#include "game/Emitter.hpp"
#include "game/Fighter.hpp"
#include "game/HitBlob.hpp"
#include "game/VisualEffect.hpp"
#include "game/World.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

FighterActionDef::FighterActionDef(const FighterDef& fighter, SmallString name)
    : fighter(fighter), name(name) {}

FighterActionDef::~FighterActionDef()
{
    if (scriptClass) wrenReleaseHandle(fighter.world.vm, scriptClass);
}

//============================================================================//

void FighterActionDef::load_json_from_file()
{
    blobs.clear();
    effects.clear();
    emitters.clear();

    const String path = "assets/{}/actions/{}.json"_format(fighter.directory, name);

    const auto root = sq::try_parse_json_from_file(path);
    if (root.has_value() == false)
    {
        sq::log_warning("'{}/actions/{}': missing json", fighter.directory, name);
        return;
    }

    String errors;

    TRY_FOR (const auto& item : root->at("blobs").items())
    {
        HitBlobDef& blob = blobs[item.key()];

        try { blob.from_json(item.value(), fighter.armature); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\nblob '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    TRY_FOR (const auto& item : root->at("effects").items())
    {
        VisualEffectDef& effect = effects[item.key()];

        try { effect.from_json(item.value(), fighter.armature, fighter.world.caches.effects); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\neffect '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    TRY_FOR (const auto& item : root->at("emitters").items())
    {
        Emitter& emitter = emitters[item.key()];

        try { emitter.from_json(item.value(), fighter.armature); }
        catch (const std::exception& ex) {
            sq::format_append(errors, "\nemitter '{}': {}", item.key(), ex.what());
        }
    }
    CATCH (const std::exception& ex) { errors += '\n'; errors += ex.what(); }

    if (errors.empty() == false)
        sq::log_warning_multiline("'{}/actions/{}': errors in json{}", fighter.directory, name, errors);
}

//============================================================================//

void FighterActionDef::load_wren_from_file()
{
    const String path = "assets/{}/actions/{}.wren"_format(fighter.directory, name);

    // set wrenSource to either the file contents or a fallback script
    auto source = sq::try_read_text_from_file(path);
    if (source.has_value() == false)
    {
        sq::log_warning("'{}/actions/{}': missing script", fighter.directory, name);

        // use default version of this action if one exists
        source = sq::try_read_text_from_file("wren/actions/{}.wren"_format(name));
        if (source.has_value() == false)
            source = sq::read_text_from_file("wren/fallback/FighterAction.wren");
    }
    wrenSource = std::move(*source);

    // parse wrenSource and store the class handle
    interpret_module();
}

//============================================================================//

void FighterActionDef::interpret_module()
{
    auto& vm = fighter.world.vm;
    auto& editor = fighter.world.editor;

    const String module = "{}/actions/{}"_format(fighter.directory, name);

    // if the module is already loaded, unload it
    if (scriptClass != nullptr)
    {
        wrenReleaseHandle(vm, scriptClass);
        wrenUnloadModule(vm, module.c_str());
    }

    // interpret wren source string into a new module
    String errors = vm.safe_interpret(module.c_str(), wrenSource.c_str());

    // make sure that module contains a Script
    if (errors.empty() == true && wrenHasVariable(vm, module.c_str(), "Script") == false)
        errors = "module lacks a 'Script' class\n";

    // problem with the module, show error message and load fallback
    if (errors.empty() == false)
    {
        String message = "'{}/actions/{}'\n{}C++ | load_wren_from_string()"_format(fighter.directory, name, errors);

        if (editor == nullptr)
            sq::log_error_multiline(message);

        else if (editor->actionKey != std::tuple(fighter.name, name))
            sq::log_warning_multiline(message);

        else editor->errorMessage = std::move(message);

        // unload module with errors
        wrenUnloadModule(vm, module.c_str());

        // use default version of this action if one exists
        auto source = sq::try_read_text_from_file("wren/actions/{}.wren"_format(name));
        if (source.has_value() == false)
            source = sq::read_text_from_file("wren/fallback/FighterAction.wren");

        // default and fallback scripts are assumed not to have errors
        vm.interpret(module.c_str(), source->c_str());
    }

    // no errors, clear editor error message
    else if (editor != nullptr && editor->actionKey == std::tuple{fighter.name, name})
        editor->errorMessage.clear();

    // store the class for use by FighterAction
    scriptClass = vm.get_variable(module.c_str(), "Script");

    // don't need the source anymore, so free some memory
    if (editor == nullptr) wrenSource = String();
}

//============================================================================//

bool FighterActionDef::has_changes(const FighterActionDef& other) const
{
    if (blobs != other.blobs) return true;
    if (effects != other.effects) return true;
    if (emitters != other.emitters) return true;
    if (wrenSource != other.wrenSource) return true;
    return false;
}

void FighterActionDef::apply_changes(const FighterActionDef& other)
{
    blobs.clear();
    for (const auto& [key, blob] : other.blobs)
        blobs.try_emplace(key, blob);

    effects.clear();
    for (const auto& [key, blob] : other.effects)
        effects.try_emplace(key, blob);

    emitters.clear();
    for (const auto& [key, emitter] : other.emitters)
        emitters.try_emplace(key, emitter);

    wrenSource = other.wrenSource;
}

std::unique_ptr<FighterActionDef> FighterActionDef::clone() const
{
    auto result = std::make_unique<FighterActionDef>(fighter, name);

    result->apply_changes(*this);

    return result;
}

//============================================================================//

FighterAction::FighterAction(const FighterActionDef& def, Fighter& fighter)
    : def(def), fighter(fighter), world(fighter.world)
{
    initialise_script();
}

FighterAction::~FighterAction()
{
    if (mScriptHandle) wrenReleaseHandle(world.vm, mScriptHandle);
    if (mFiberHandle) wrenReleaseHandle(world.vm, mFiberHandle);
}

void FighterAction::initialise_script()
{
    // release existing handles
    if (mScriptHandle) wrenReleaseHandle(world.vm, mScriptHandle);
    if (mFiberHandle) wrenReleaseHandle(world.vm, mFiberHandle);
    mScriptHandle = mFiberHandle = nullptr;

    // create a new instance of the Script object
    const auto safe = world.vm.safe_call<WrenHandle*>(world.handles.new_1, def.scriptClass, this);
    if (safe.ok == false)
    {
        set_error_message("initialise_script", safe.error);

        world.vm.load_module("fallback/FighterAction");

        mScriptHandle = world.vm.call<WrenHandle*> (
            world.handles.new_1, wren::GetVar("fallback/FighterAction", "Script"), this
        );
    }
    else mScriptHandle = safe.value;
}

//============================================================================//

void FighterAction::call_do_start()
{
    SQASSERT(fighter.activeAction == this, "action not active");

    const auto error = fighter.world.vm.safe_call_void(fighter.world.handles.action_do_start, this);
    if (error.empty() == false)
        set_error_message("call_do_start", error);
}

void FighterAction::call_do_updates()
{
    SQASSERT(fighter.activeAction == this, "action not active");

    const auto error = fighter.world.vm.safe_call_void(fighter.world.handles.action_do_updates, this);
    if (error.empty() == false)
        set_error_message("call_do_updates", error);
}

void FighterAction::call_do_cancel()
{
    SQASSERT(fighter.activeAction == this, "action not active");

    const auto error = fighter.world.vm.safe_call_void(fighter.world.handles.action_do_cancel, this);
    if (error.empty() == false)
        set_error_message("call_do_cancel", error);
}

//============================================================================//

void FighterAction::set_error_message(StringView method, StringView errors)
{
    String message = "'{}/actions/{}'\n{}C++ | {}()\n"_format(def.fighter.directory, def.name, errors, method);

    if (world.editor == nullptr)
        sq::log_error_multiline(message);

    else if (world.editor->actionKey != std::tuple(def.fighter.name, def.name))
        sq::log_warning_multiline(message);

    // only show the first error, which usually causes the other errors
    else if (world.editor->errorMessage.empty())
        world.editor->errorMessage = std::move(message);
}
