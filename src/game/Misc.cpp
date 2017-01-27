#include <sqee/misc/Json.hpp>

#include "Misc.hpp"

using namespace sts;

//============================================================================//

void sts::misc::load_actions_from_json(Fighter& fighter)
{
    fighter.actions.neutral_first = std::make_unique<Action>(Action::Type::Neutral_First);
    fighter.actions.neutral_second = std::make_unique<Action>(Action::Type::Neutral_Second);
    fighter.actions.neutral_third = std::make_unique<Action>(Action::Type::Neutral_Third);

    fighter.actions.tilt_down = std::make_unique<Action>(Action::Type::Tilt_Down);
    fighter.actions.tilt_forward = std::make_unique<Action>(Action::Type::Tilt_Forward);
    fighter.actions.tilt_up = std::make_unique<Action>(Action::Type::Tilt_Up);

    const string path = "assets/fighters/" + fighter.mName + "/attacks/";

    const auto load_action = [&](unique_ptr<Action>& action, string str)
    {
        const auto json = sq::parse_json(path + str + ".json");

        action->props.message = json.at("message");
        action->props.time = json.at("time");
    };

    load_action(fighter.actions.neutral_first, "neutral_first");
    load_action(fighter.actions.neutral_second, "neutral_second");
    load_action(fighter.actions.neutral_third, "neutral_third");

    load_action(fighter.actions.tilt_down, "tilt_down");
    load_action(fighter.actions.tilt_forward, "tilt_forward");
    load_action(fighter.actions.tilt_up, "tilt_up");
}
