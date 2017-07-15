#include "Sara_Actions.hpp"

using namespace sts;

//============================================================================//

unique_ptr<Actions> sts::create_actions(Sara_Fighter& fighter)
{
    auto actions = std::make_unique<Actions>();

    actions->neutral_first = std::make_unique<DumbAction>("Sara Neutral_First");
    actions->tilt_down     = std::make_unique<DumbAction>("Sara Tilt_Down");
    actions->tilt_forward  = std::make_unique<DumbAction>("Sara Tilt_Forward");
    actions->tilt_up       = std::make_unique<DumbAction>("Sara Tilt_Up");
    actions->air_back      = std::make_unique<DumbAction>("Sara Air_Back");
    actions->air_down      = std::make_unique<DumbAction>("Sara Air_Down");
    actions->air_forward   = std::make_unique<DumbAction>("Sara Air_Forward");
    actions->air_neutral   = std::make_unique<DumbAction>("Sara Air_Neutral");
    actions->air_up        = std::make_unique<DumbAction>("Sara Air_Up");
    actions->dash_attack   = std::make_unique<DumbAction>("Sara Dash_Attack");

    return actions;
}
