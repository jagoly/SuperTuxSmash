#pragma once

#include "fighters/Tux_Fighter.hpp"

//============================================================================//

namespace sts {

unique_ptr<Actions> create_actions(FightWorld& world, Tux_Fighter& fighter);

} // namespace sts
