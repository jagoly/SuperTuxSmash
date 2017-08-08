#pragma once

#include "fighters/Sara_Fighter.hpp"

//============================================================================//

namespace sts {

unique_ptr<Actions> create_actions(FightWorld& world, Sara_Fighter& fighter);

} // namespace sts
