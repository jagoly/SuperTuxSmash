#include <sqee/assert.hpp>
#include <sqee/misc/Json.hpp>

#include "Fighter.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Fighter::Fighter(string name) : mName(name)
{
    const auto json = sq::parse_json("assets/fighters/" + name + "/fighter.json");

    stats.walk_speed    = json.at("walk_speed");
    stats.dash_speed    = json.at("dash_speed");
    stats.air_speed     = json.at("air_speed");
    stats.land_traction = json.at("land_traction");
    stats.air_traction  = json.at("air_traction");
    stats.hop_height    = json.at("hop_height");
    stats.jump_height   = json.at("jump_height");
    stats.fall_speed    = json.at("fall_speed");

    state.action = State::Action::None;
    state.move = State::Move::None;
    state.direction = State::Direction::Left;
}

//============================================================================//

void Fighter::impl_update_before()
{
    //========================================================//

    // apply friction /////

    if (state.move == State::Move::Walking)
    {
        if (mVelocity.x < -0.f) mVelocity.x = maths::min(mVelocity.x + (stats.land_traction * 0.5f), -0.f);
        if (mVelocity.x > +0.f) mVelocity.x = maths::max(mVelocity.x - (stats.land_traction * 0.5f), +0.f);
    }

    if (state.move == State::Move::Dashing)
    {
        if (mVelocity.x < -0.f) mVelocity.x = maths::min(mVelocity.x + (stats.land_traction * 0.5f), -0.f);
        if (mVelocity.x > +0.f) mVelocity.x = maths::max(mVelocity.x - (stats.land_traction * 0.5f), +0.f);
    }

    if (state.move == State::Move::Jumping)
    {
        if (mVelocity.x < -0.f) mVelocity.x = maths::min(mVelocity.x + (stats.air_traction * 0.2f), -0.f);
        if (mVelocity.x > +0.f) mVelocity.x = maths::max(mVelocity.x - (stats.air_traction * 0.2f), +0.f);
    }

    //========================================================//

    // apply gravity /////

    if (state.move == State::Move::Jumping)
    {
        const float fraction = (2.f/3.f) * stats.hop_height / stats.jump_height;

        float gravity = stats.fall_speed * 0.5f;
        if (!mJumpHeld && mVelocity.y > 0.f) gravity /= fraction;
        mVelocity.y -= gravity;

        const float maxFall = stats.fall_speed * 15.f;
        mVelocity.y = maths::max(mVelocity.y, -maxFall);
    }

    //========================================================//

    // update active actions

    if (actions.active != nullptr)
    {
        if (actions.active->on_tick() == true)
        {
            actions.active->on_finish();
            actions.active = nullptr;

            state.action = State::Action::None;
        }
    }
}

//============================================================================//

void Fighter::impl_input_actions(Controller::Input input)
{
    //========================================================//
    switch (state.action) {
    //========================================================//

    case State::Action::None:
    {
        if (input.press_attack == true)
        {
            if (state.move == State::Move::None || state.move == State::Move::Walking)
            {
                if (maths::length(input.axis_move) <= 0.2f)
                {
                    state.action = State::Action::Neutral;
                    actions.active = actions.neutral_first.get();
                    actions.active->on_start();
                }
                else
                {
                    state.action = State::Action::Tilt;

                    if (std::abs(input.axis_move.x) > std::abs(input.axis_move.y))
                        actions.active = actions.tilt_forward.get();

                    else if (input.axis_move.y < -0.f)
                        actions.active = actions.tilt_down.get();

                    else if (input.axis_move.y > +0.f)
                        actions.active = actions.tilt_up.get();

                    else SQASSERT(false, "");

                    actions.active->on_start();
                }
            }
        }

        break;
    }

    case State::Action::Neutral:
    {
        break;
    }

    //========================================================//
    } // switch (state.attack)
    //========================================================//
}

//============================================================================//

void Fighter::impl_input_movement(Controller::Input input)
{
    //========================================================//

    // maximum horizontal velocities (signed)

    const float maxNewWalkSpeed = input.axis_move.x * stats.walk_speed * 5.f;
    const float maxNewDashSpeed = input.axis_move.x * stats.dash_speed * 8.f;
    const float maxNewAirSpeed = input.axis_move.x * stats.air_speed * 5.f;

    //========================================================//
    switch (state.move) {
    //========================================================//

    case State::Move::None:
    {
        if (input.axis_move.x < -0.f)
        {
            state.move = State::Move::Walking;
            state.direction = State::Direction::Left;
            mVelocity.x = -(stats.land_traction * 0.5f);
        }

        if (input.axis_move.x > +0.f)
        {
            state.move = State::Move::Walking;
            state.direction = State::Direction::Right;
            mVelocity.x = +(stats.land_traction * 0.5f);
        }

        if (input.activate_dash == true)
        {
            state.move = State::Move::Dashing;
        }

        if (input.press_jump == true)
        {
            state.move = State::Move::Jumping;

            mJumpHeld = true;

            const float base = stats.fall_speed * stats.jump_height;
            mVelocity.y = std::sqrt(base * 3.f * 48.f);
        }

        break;
    }

    //========================================================//

    case State::Move::Walking:
    {
        if (input.axis_move.x < -0.f)
        {
            state.direction = State::Direction::Left;

            if (mVelocity.x > maxNewWalkSpeed)
            {
                mVelocity.x -= stats.land_traction * 1.f;
                mVelocity.x = maths::max(mVelocity.x, maxNewWalkSpeed);
            }
        }

        if (input.axis_move.x > +0.f)
        {
            state.direction = State::Direction::Right;

            if (mVelocity.x < maxNewWalkSpeed)
            {
                mVelocity.x += stats.land_traction * 1.f;
                mVelocity.x = maths::min(mVelocity.x, maxNewWalkSpeed);
            }
        }

        if (input.activate_dash == true)
        {
            state.move = State::Move::Dashing;
        }

        if (input.press_jump == true)
        {
            state.move = State::Move::Jumping;

            mJumpHeld = true;

            const float base = stats.fall_speed * stats.jump_height;
            mVelocity.y = std::sqrt(base * 3.f * 48.f);
        }

        break;
    }

    //========================================================//

    case State::Move::Dashing:
    {
        if (input.axis_move.x < -0.f)
        {
            if (mVelocity.x > maxNewDashSpeed)
            {
                mVelocity.x -= stats.land_traction * 1.f * 1.5f;
                mVelocity.x = maths::max(mVelocity.x, maxNewDashSpeed);
            }
        }

        if (input.axis_move.x > +0.f)
        {
            if (mVelocity.x < maxNewDashSpeed)
            {
                mVelocity.x += stats.land_traction * 1.f * 1.5f;
                mVelocity.x = maths::min(mVelocity.x, maxNewDashSpeed);
            }
        }

        if (std::abs(input.axis_move.x) < 0.8f)
        {
            state.move = State::Move::Walking;
        }

        if (input.press_jump == true)
        {
            state.move = State::Move::Jumping;

            mJumpHeld = true;

            const float base = stats.fall_speed * stats.jump_height;
            mVelocity.y = std::sqrt(base * 3.f * 48.f);
        }

        break;
    }

    //========================================================//

    case State::Move::Jumping:
    {
        mJumpHeld = mJumpHeld && input.hold_jump;

        if (input.axis_move.x < -0.f)
        {
            if (mVelocity.x > maxNewAirSpeed)
            {
                mVelocity.x -= stats.air_traction * 0.4f;
                mVelocity.x = maths::max(mVelocity.x, maxNewAirSpeed);
            }
        }

        if (input.axis_move.x > +0.f)
        {
            if (mVelocity.x < maxNewAirSpeed)
            {
                mVelocity.x += stats.air_traction * 0.4f;
                mVelocity.x = maths::min(mVelocity.x, maxNewAirSpeed);
            }
        }

        if (input.axis_move.y < -0.f)
        {
            const float base = stats.jump_height * stats.fall_speed;
            mVelocity.y += base * input.axis_move.y * 0.1f;
        }

        if (input.axis_move.y > +0.f)
        {
            const float base = stats.jump_height * stats.fall_speed;
            mVelocity.y += base * input.axis_move.y * 0.1f;
        }

        break;
    }

    //========================================================//

    case State::Move::Falling:
    {
        break;
    }

    //========================================================//
    } // switch (state.move)
    //========================================================//
}

//============================================================================//

void Fighter::impl_update_after()
{
    //========================================================//

    // intergrate the state of the fighter

    previous = current; // flip interpolation frame

    current.position = current.position + (mVelocity / 48.f);

    //========================================================//
    switch (state.move) {
    //========================================================//

    case State::Move::None:
    {
        break;
    }

    case State::Move::Walking:
    {
        if (mVelocity.x == 0.f)
        {
            state.move = State::Move::None;
            // running -> standing
        }

        break;
    }

    case State::Move::Jumping:
    {
        if (current.position.y <= 0.f)
        {
            // set velocity and position to zero
            mVelocity.y = current.position.y = 0.f;

            if (mVelocity.x == 0.f)
            {
                state.move = State::Move::None;
                // jumping -> standing
            }

            if (mVelocity.x != 0.f)
            {
                state.move = State::Move::Walking;
                // jumping -> running
            }
        }

        break;
    }

    case State::Move::Falling:
    {
        break;
    }

    //========================================================//
    } // switch (state.move)
    //========================================================//
}

//============================================================================//

void Fighter::impl_tick_base()
{
    const auto input = mController->get_input();

    this->impl_validate_stats();

    this->impl_update_before();

    this->impl_input_actions(input);
    this->impl_input_movement(input);

    this->impl_update_after();
}

//============================================================================//

void Fighter::impl_validate_stats()
{
    SQASSERT(stats.walk_speed / stats.dash_speed < (8.f / 5.f), "invalid walk/dash ratio");
    SQASSERT(stats.hop_height / stats.jump_height < (3.f / 2.f), "invalid hop/jump ratio");
}
