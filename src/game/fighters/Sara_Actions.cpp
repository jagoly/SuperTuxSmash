#include <sqee/debug/Logging.hpp>

#include "Sara_Actions.hpp"

using namespace sts;

//============================================================================//

namespace sts::actions {

//----------------------------------------------------------------------------//

struct Sara_Neutral_First final : public Action
{
    Sara_Neutral_First()
    {
        add_frame_method(4u) = []() { sq::log_only("hello from frame 4 of sara neutral first"); };
        add_frame_method(10u) = []() { sq::log_only("bonjour à partir du cadre 10 de sara neutre d'abord"); };
    }

    bool on_tick() override { return get_current_frame() == 20u; }
};

//----------------------------------------------------------------------------//

} // namespace sts::actions

//============================================================================//

unique_ptr<Actions> sts::create_actions(Sara_Fighter& fighter)
{
    auto actions = std::make_unique<Actions>();

    actions->neutral_first = std::make_unique<actions::Sara_Neutral_First>();
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
