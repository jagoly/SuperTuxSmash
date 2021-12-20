#include "game/FighterState.hpp"

#include "main/Options.hpp"

#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

FighterState::FighterState(Fighter& fighter, SmallString name)
    : fighter(fighter), world(fighter.world), name(name)
{
}

FighterState::~FighterState()
{
    if (mScriptHandle) wrenReleaseHandle(world.vm, mScriptHandle);
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

void FighterState::load_wren_from_file()
{
    SQASSERT(mScriptHandle == nullptr, "script already loaded");

    // first try to load a fighter specific script
    auto source = sq::try_read_text_from_file("assets/fighters/{}/states/{}.wren"_format(fighter.name, name));

    // otherwise just use the common script
    if (source.has_value() == false)
        source = sq::read_text_from_file("wren/states/{}.wren"_format(name));

    // every fighter gets its own module for each state
    // todo: probably not needed for states?
    const auto module = "{}_STATE_{}"_format(fighter.index, name);

    // interpret wren source into the new module
    world.vm.interpret(module.c_str(), source->c_str());

    // create a new instance of the Script object
    mScriptHandle = world.vm.call<WrenHandle*>(world.handles.new_1, wren::GetVar(module.c_str(), "Script"), this);
}

//============================================================================//

void FighterState::set_error_message(StringView method, StringView error)
{
    String message = "State '{}/{}'\n{}\nC++ | {}()\n"_format(fighter.name, name, error, method);

    if (world.options.editor_mode == false)
        sq::log_error_multiline(message);

    fighter.set_error_message(this, std::move(message));
}
