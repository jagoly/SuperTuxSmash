#include "fighters/Sara_Actions.hpp"
#include "fighters/Sara_Fighter.hpp"

using namespace sts;

//============================================================================//

Sara_Fighter::Sara_Fighter(uint8_t index, FightWorld& world, Controller& controller)
    : Fighter(index, world, controller, "assets/fighters/Sara/")
{
    mActions = create_actions(mFightWorld, *this);

    //--------------------------------------------------------//

    mLocalDiamond.xNeg = { -0.3f, 0.8f };
    mLocalDiamond.xPos = { +0.3f, 0.8f };
    mLocalDiamond.yNeg = { 0.f, 0.f };
    mLocalDiamond.yPos = { 0.f, 1.4f };
}

//============================================================================//

void Sara_Fighter::tick()
{
    base_tick_fighter();

    base_tick_animation();
}
