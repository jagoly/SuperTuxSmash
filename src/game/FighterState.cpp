#include "game/FighterState.hpp"

#include "game/Fighter.hpp"
#include "game/World.hpp"

#include <sqee/misc/Files.hpp>

using namespace sts;

// FighterState is much simpler than FighterAction, since the editor only
// supports editing actions, so states don't need robust error handling.

// This may change in the future, but it would be a bit of work.

//============================================================================//

FighterStateDef::FighterStateDef(const FighterDef& fighter, TinyString name)
    : fighter(fighter), name(name) {}

FighterStateDef::~FighterStateDef()
{
    if (scriptClass) wrenReleaseHandle(fighter.world.vm, scriptClass);
}

//============================================================================//

void FighterStateDef::load_wren_from_file()
{
    SQASSERT(scriptClass == nullptr, "module already loaded");

    auto& vm = fighter.world.vm;

    // first try to load a fighter specific script
    auto source = sq::try_read_text_from_file(fmt::format("assets/{}/states/{}.wren", fighter.directory, name));

    String module;

    // for states, per fighter scripts are not required
    if (source.has_value() == false)
    {
        module = fmt::format("states/{}", name);
        vm.load_module(module.c_str());
    }
    else
    {
        module = fmt::format("{}/states/{}", fighter.directory, name);
        vm.interpret(module.c_str(), source->c_str());
    }

    // store the class for use by FighterState
    scriptClass = vm.get_variable(module.c_str(), "Script");
}

//============================================================================//

FighterState::FighterState(const FighterStateDef& def, Fighter& fighter)
    : def(def), fighter(fighter), world(fighter.world)
{
    // create a new instance of the Script object
    mScriptHandle = world.vm.call<WrenHandle*>(world.handles.new_1, def.scriptClass, this);
}

FighterState::~FighterState()
{
    wrenReleaseHandle(world.vm, mScriptHandle);
}

//============================================================================//

void FighterState::call_do_enter()
{
    SQASSERT(fighter.activeState == this, "state not active");

    const auto error = world.vm.safe_call_void(world.handles.state_do_enter, this);
    if (error.empty() == false)
        set_error_message("call_do_enter", error);
}

//============================================================================//

void FighterState::call_do_updates()
{
    SQASSERT(fighter.activeState == this, "state not active");

    const auto error = world.vm.safe_call_void(world.handles.state_do_updates, this);
    if (error.empty() == false)
        set_error_message("call_do_updates", error);
}

//============================================================================/

void FighterState::call_do_exit()
{
    SQASSERT(fighter.activeState == this, "state not active");

    const auto error = world.vm.safe_call_void(world.handles.state_do_exit, this);
    if (error.empty() == false)
        set_error_message("call_do_exit", error);
}

//============================================================================//

void FighterState::set_error_message(StringView method, StringView errors)
{
    String message = fmt::format("'{}/states/{}'\n{}C++ | {}()\n", def.fighter.directory, def.name, errors, method);

    if (world.editor == nullptr)
        sq::log_error_multiline(message);

    else
        sq::log_warning_multiline(message);
}
