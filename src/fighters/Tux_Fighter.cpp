#include "fighters/Tux_Fighter.hpp"

using namespace sts;

//============================================================================//

Tux_Fighter::Tux_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, "Tux")
{
    mLocalDiamond.xNeg = { -0.4f, 0.5f };
    mLocalDiamond.xPos = { +0.4f, 0.5f };
    mLocalDiamond.yNeg = { 0.f, 0.f };
    mLocalDiamond.yPos = { 0.f, 1.0f };
}

//============================================================================//

void Tux_Fighter::tick()
{
    base_tick_fighter();

    base_tick_animation();
}
