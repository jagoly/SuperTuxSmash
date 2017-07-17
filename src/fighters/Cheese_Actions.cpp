#include "fighters/Cheese_Actions.hpp"

//============================================================================//

unique_ptr<sts::Actions> sts::create_actions(FightSystem& system, Cheese_Fighter& fighter)
{
    auto actions = std::make_unique<Actions>();

    actions->neutral_first = std::make_unique<DumbAction>(system, fighter, "Cheese Neutral_First");
    actions->tilt_down     = std::make_unique<DumbAction>(system, fighter, "Cheese Tilt_Down");
    actions->tilt_forward  = std::make_unique<DumbAction>(system, fighter, "Cheese Tilt_Forward");
    actions->tilt_up       = std::make_unique<DumbAction>(system, fighter, "Cheese Tilt_Up");
    actions->air_back      = std::make_unique<DumbAction>(system, fighter, "Cheese Air_Back");
    actions->air_down      = std::make_unique<DumbAction>(system, fighter, "Cheese Air_Down");
    actions->air_forward   = std::make_unique<DumbAction>(system, fighter, "Cheese Air_Forward");
    actions->air_neutral   = std::make_unique<DumbAction>(system, fighter, "Cheese Air_Neutral");
    actions->air_up        = std::make_unique<DumbAction>(system, fighter, "Cheese Air_Up");
    actions->dash_attack   = std::make_unique<DumbAction>(system, fighter, "Cheese Dash_Attack");

    return actions;
}
