#pragma once

#include "fighters/Cheese_Fighter.hpp"

//============================================================================//

namespace sts {

unique_ptr<Actions> create_actions(FightSystem& system, Cheese_Fighter& fighter);

} // namespace sts
