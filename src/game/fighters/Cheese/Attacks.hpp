#pragma once

#include <game/fighters/Cheese/Fighter.hpp>

namespace sts { namespace fighters {

//============================================================================//

struct Cheese_Attacks : public Attacks
{
    Cheese_Attacks(Cheese_Fighter& fighter);
};

//============================================================================//

}} // namespace sts::fighters
