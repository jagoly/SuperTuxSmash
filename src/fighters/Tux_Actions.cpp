#include "fighters/Tux_Actions.hpp"

//============================================================================//

namespace sts::actions {

//----------------------------------------------------------------------------//

struct Tux_Base : public BaseAction<Tux_Fighter>
{
    using BaseAction<Tux_Fighter>::BaseAction;
};

//----------------------------------------------------------------------------//

} // namespace sts::actions

//============================================================================//

unique_ptr<sts::Actions> sts::create_actions(FightWorld& world, Tux_Fighter& fighter)
{
    auto actions = std::make_unique<sts::Actions>();

    actions->neutral_first = std::make_unique<DumbAction>(world, fighter, "Tux Neutral_First");
    actions->tilt_down     = std::make_unique<DumbAction>(world, fighter, "Tux Tilt_Down");
    actions->tilt_forward  = std::make_unique<DumbAction>(world, fighter, "Tux Tilt_Forward");
    actions->tilt_up       = std::make_unique<DumbAction>(world, fighter, "Tux Tilt_Up");
    actions->air_back      = std::make_unique<DumbAction>(world, fighter, "Tux Air_Back");
    actions->air_down      = std::make_unique<DumbAction>(world, fighter, "Tux Air_Down");
    actions->air_forward   = std::make_unique<DumbAction>(world, fighter, "Tux Air_Forward");
    actions->air_neutral   = std::make_unique<DumbAction>(world, fighter, "Tux Air_Neutral");
    actions->air_up        = std::make_unique<DumbAction>(world, fighter, "Tux Air_Up");
    actions->dash_attack   = std::make_unique<DumbAction>(world, fighter, "Tux Dash_Attack");

    actions->load_json("Tux");

    return actions;
}
