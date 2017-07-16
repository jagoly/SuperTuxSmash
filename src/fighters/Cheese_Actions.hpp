#pragma once

#include "fighters/Cheese_Fighter.hpp"

//============================================================================//

namespace sts {

unique_ptr<Actions> create_actions(Cheese_Fighter& fighter);

} // namespace sts
