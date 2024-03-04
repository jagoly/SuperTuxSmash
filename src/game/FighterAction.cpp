#include "game/FighterAction.hpp"

#include "game/Emitter.hpp"
#include "game/Fighter.hpp"
#include "game/HitBlob.hpp"
#include "game/VisualEffect.hpp"
#include "game/World.hpp"

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

    const auto document = JsonDocument::parse_file(fmt::format("assets/{}/actions/{}.json", fighter.directory, name));
    const auto json = document.root().as<JsonObject>();

    fmt::memory_buffer errors;

    const auto objects_from_json = [&json, &errors](StringView mapKey, auto& map, auto&... fromJsonArgs)
    {
        for (const auto [key, jObject] : json[mapKey].as<JsonObject>())
        {
            try {
                map[key].from_json(jObject.as<JsonObject>(), fromJsonArgs...);
            }
            catch (const std::exception& ex) {
                fmt::format_to(fmt::appender(errors), "\n{}", ex.what());
            }
        }
    };

    objects_from_json("blobs", blobs, fighter.armature);
    objects_from_json("effects", effects, fighter.armature, fighter.world.caches.effects);
    objects_from_json("emitters", emitters, fighter.armature);

    if (errors.size() != 0u)
        sq::log_warning_multiline("'{}/actions/{}': errors in json{}", fighter.directory, name, StringView(errors.data(), errors.size()));
}

//============================================================================//

void FighterActionDef::load_wren_from_file()
{
    // set wrenSource to either the file contents or a fallback script
    auto source = sq::try_read_text_from_file(fmt::format("assets/{}/actions/{}.wren", fighter.directory, name));
    if (source.has_value() == false)
    {
        sq::log_warning("'{}/actions/{}': missing script", fighter.directory, name);

        // use default version of this action if one exists
        source = sq::try_read_text_from_file(fmt::format("wren/actions/{}.wren", name));
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

    const String module = fmt::format("{}/actions/{}", fighter.directory, name);

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
        String message = fmt::format("'{}/actions/{}'\n{}C++ | load_wren_from_string()", fighter.directory, name, errors);

        if (editor == nullptr)
            sq::log_error_multiline(message);

        else if (editor->ctxKey != module)
            sq::log_warning_multiline(message);

        else editor->errorMessage = std::move(message);

        // unload module with errors
        wrenUnloadModule(vm, module.c_str());

        // use default version of this action if one exists
        auto source = sq::try_read_text_from_file(fmt::format("wren/actions/{}.wren", name));
        if (source.has_value() == false)
            source = sq::read_text_from_file("wren/fallback/FighterAction.wren");

        // default and fallback scripts are assumed not to have errors
        vm.interpret(module.c_str(), source->c_str());
    }

    // no errors, clear editor error message
    else if (editor != nullptr && editor->ctxKey == module)
        editor->errorMessage.clear();

    // store the class for use by FighterAction
    scriptClass = vm.get_variable(module.c_str(), "Script");

    // don't need the source anymore, so free some memory
    if (editor == nullptr) wrenSource = String();
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
    String message = fmt::format (
        "'{}/actions/{}'\n{}C++ | {}() | frame = {}", def.fighter.directory, def.name, errors, method, mCurrentFrame
    );

    if (world.editor == nullptr)
        sq::log_error_multiline(message);

    else if (world.editor->ctxKey != fmt::format("{}/actions/{}", def.fighter.directory, def.name))
        sq::log_warning_multiline(message);

    // only show the first error, which usually causes the other errors
    else if (world.editor->errorMessage.empty())
        world.editor->errorMessage = std::move(message);
}
