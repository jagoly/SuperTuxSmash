#include "fighters/Cheese_Actions.hpp"
#include "fighters/Cheese_Fighter.hpp"

using namespace sts;

//============================================================================//

Cheese_Fighter::Cheese_Fighter(Controller& controller) : Fighter("Cheese", controller)
{
    actions = create_actions(*this);
}

//============================================================================//

void Cheese_Fighter::tick()
{
    this->base_tick_entity();
    this->base_tick_fighter();

    //--------------------------------------------------------//

    if (state.move == State::Move::None)
        colour = Vec3F(1.f, 0.f, 0.f);

    if (state.move == State::Move::Walking)
        colour = Vec3F(0.f, 1.f, 0.f);

    if (state.move == State::Move::Dashing)
        colour = Vec3F(0.3f, 0.3f, 0.6f);

    if (state.move == State::Move::Jumping)
        colour = Vec3F(0.f, 0.f, 1.f);

    if (state.move == State::Move::Falling)
        colour = Vec3F(0.1f, 0.1f, 2.f);
}
