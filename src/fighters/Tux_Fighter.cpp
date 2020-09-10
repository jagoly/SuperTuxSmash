#include "fighters/Tux_Fighter.hpp"

using namespace sts;

//============================================================================//

Tux_Fighter::Tux_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, FighterEnum::Tux) {}

//============================================================================//

void Tux_Fighter::tick()
{
    base_tick_fighter();
}
