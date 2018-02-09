#include "fighters/Tux_Fighter.hpp"

using namespace sts;

//============================================================================//

Tux_Fighter::Tux_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, "Tux")
{
    mLocalDiamond.offsetTop = 1.f;
    mLocalDiamond.offsetMiddle = 0.5f;
    mLocalDiamond.halfWidth = 0.4f;
}

//============================================================================//

void Tux_Fighter::tick()
{
    base_tick_fighter();

    base_tick_animation();
}
