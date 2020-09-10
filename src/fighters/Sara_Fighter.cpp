#include "fighters/Sara_Fighter.hpp"

using namespace sts;

//============================================================================//

Sara_Fighter::Sara_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, FighterEnum::Sara) {}

//============================================================================//

void Sara_Fighter::tick()
{
    base_tick_fighter();
}
