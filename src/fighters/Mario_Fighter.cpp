#include "fighters/Mario_Fighter.hpp"

using namespace sts;

//============================================================================//

Mario_Fighter::Mario_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, FighterEnum::Mario) {}

//============================================================================//

void Mario_Fighter::tick()
{
    base_tick_fighter();
}
