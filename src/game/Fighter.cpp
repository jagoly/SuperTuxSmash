#include <sqee/assert.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/misc/StringCast.hpp>

#include "Fighter.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Fighter::Fighter(const string& name, Controller& controller)
    : Entity(name), controller(controller)
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

    state.move = State::Move::None;
    state.direction = State::Direction::Left;
}

//============================================================================//

void Fighter::impl_input_movement(Controller::Input input)
{
    //========================================================//

    // maximum horizontal velocities (signed)

    const float maxNewWalkSpeed = input.axis_move.x * (stats.walk_speed * 5.f + stats.land_traction * 0.5f);
    const float maxNewAirSpeed = input.axis_move.x * (stats.air_speed * 5.f + stats.air_traction * 0.2f);
    const float maxDashSpeed = stats.dash_speed * 8.f + stats.land_traction * 0.5f;

    //========================================================//
    switch (state.move) {
    //========================================================//

    case State::Move::None:
    {
        if (actions->active.type != Actions::Type::None) break;

        //========================================================//

        if (input.axis_move.x < -0.f)
        {
            state.move = State::Move::Walking;
            state.direction = State::Direction::Left;
            mVelocity.x = -(stats.land_traction * 1.f);
        }

        if (input.axis_move.x > +0.f)
        {
            state.move = State::Move::Walking;
            state.direction = State::Direction::Right;
            mVelocity.x = +(stats.land_traction * 1.f);
        }

        //========================================================//

        if (input.press_jump == true)
        {
            state.move = State::Move::Jumping;

            mJumpHeld = true;

            const float base = stats.fall_speed * stats.jump_height;
            mVelocity.y = std::sqrt(base * 3.f * 48.f);
        }

        else if (input.activate_dash == true)
        {
            state.move = State::Move::Dashing;
        }

        //========================================================//

        break;
    }

    //========================================================//

    case State::Move::Walking:
    {
        SQASSERT(actions->active.type == Actions::Type::None, "");

        //========================================================//

        if (input.axis_move.x == 0.f)
        {
            state.move = State::Move::None;
        }

        else if (input.axis_move.x < -0.f)
        {
            state.direction = State::Direction::Left;

            if (mVelocity.x > maxNewWalkSpeed)
            {
                mVelocity.x -= stats.land_traction * 1.f;
                mVelocity.x = maths::max(mVelocity.x, maxNewWalkSpeed);
            }
        }

        else if (input.axis_move.x > +0.f)
        {
            state.direction = State::Direction::Right;

            if (mVelocity.x < maxNewWalkSpeed)
            {
                mVelocity.x += stats.land_traction * 1.f;
                mVelocity.x = maths::min(mVelocity.x, maxNewWalkSpeed);
            }
        }

        //========================================================//

        if (input.press_jump == true)
        {
            state.move = State::Move::Jumping;

            mJumpHeld = true;

            const float base = stats.fall_speed * stats.jump_height;
            mVelocity.y = std::sqrt(base * 3.f * 48.f);
        }

        else if (input.activate_dash == true)
        {
            state.move = State::Move::Dashing;
        }

        //========================================================//

        break;
    }

    //========================================================//

    case State::Move::Dashing:
    {
        SQASSERT(actions->active.type == Actions::Type::None, "");

        //========================================================//

        if (input.axis_move.x == 0.f)
        {
            state.move = State::Move::None;
        }

        else if (input.axis_move.x < -0.f)
        {
            if (mVelocity.x > -maxDashSpeed)
            {
                mVelocity.x -= stats.land_traction * 1.5f;
                mVelocity.x = maths::max(mVelocity.x, -maxDashSpeed);
            }

            if (input.axis_move.x > -0.8f) state.move = State::Move::Walking;
        }

        else if (input.axis_move.x > +0.f)
        {
            if (mVelocity.x < +maxDashSpeed)
            {
                mVelocity.x += stats.land_traction * 1.5f;
                mVelocity.x = maths::min(mVelocity.x, +maxDashSpeed);
            }

            if (input.axis_move.x < +0.8f) state.move = State::Move::Walking;
        }

        //========================================================//

        if (input.press_jump == true)
        {
            state.move = State::Move::Jumping;

            mJumpHeld = true;

            const float base = stats.fall_speed * stats.jump_height;
            mVelocity.y = std::sqrt(base * 3.f * 48.f);
        }

        //========================================================//

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

void Fighter::impl_input_actions(Controller::Input input)
{
    if (actions->active.type == Actions::Type::None)
    {
        if (input.press_attack == true)
        {
            //========================================================//

            if (state.move == State::Move::None)
            {
                if (input.axis_move.y == 0.f) actions->active = { [&](){ return actions->fn_neutral_first(); },
                                                                  Actions::Type::Neutral_First };

                else if (input.axis_move.y < -0.f) actions->active = { [&](){ return actions->fn_tilt_down(); },
                                                                       Actions::Type::Tilt_Down };

                else if (input.axis_move.y > +0.f) actions->active = { [&](){ return actions->fn_tilt_up(); },
                                                                       Actions::Type::Tilt_Up };
            }

            //========================================================//

            else if (state.move == State::Move::Walking)
            {
                state.move = State::Move::None;

                if (std::abs(input.axis_move.y) > std::abs(input.axis_move.x))
                {
                    if (input.axis_move.y < -0.f) actions->active = { [&](){ return actions->fn_tilt_down(); },
                                                                      Actions::Type::Tilt_Down };

                    if (input.axis_move.y > +0.f) actions->active = { [&](){ return actions->fn_tilt_up(); },
                                                                      Actions::Type::Tilt_Up };
                }

                else actions->active = { [&](){ return actions->fn_tilt_forward(); },
                                         Actions::Type::Tilt_Forward };
            }

            //========================================================//

            else if (state.move == State::Move::Dashing)
            {
                state.move = State::Move::None;

                actions->active = { [&](){ return actions->fn_dash_attack(); },
                                    Actions::Type::Dash_Attack };
            }

            //========================================================//

            else if (state.move == State::Move::Jumping)
            {
                if (std::abs(input.axis_move.y) > std::abs(input.axis_move.x))
                {
                    if (input.axis_move.y < -0.f) actions->active = { [&](){ return actions->fn_air_down(); },
                                                                      Actions::Type::Air_Down };

                    if (input.axis_move.y > +0.f) actions->active = { [&](){ return actions->fn_air_up(); },
                                                                      Actions::Type::Air_Up };
                }

                else if (input.axis_move.x < -0.f && state.direction == State::Direction::Left)
                    actions->active = { [&](){ return actions->fn_air_forward(); },
                                        Actions::Type::Air_Forward };

                else if (input.axis_move.x < -0.f && state.direction == State::Direction::Right)
                    actions->active = { [&](){ return actions->fn_air_back(); },
                                        Actions::Type::Air_Back };

                else if (input.axis_move.x > +0.f && state.direction == State::Direction::Left)
                    actions->active = { [&](){ return actions->fn_air_back(); },
                                        Actions::Type::Air_Back };

                else if (input.axis_move.x > +0.f && state.direction == State::Direction::Right)
                    actions->active = { [&](){ return actions->fn_air_forward(); },
                                        Actions::Type::Air_Forward };

                else actions->active = { [&](){ return actions->fn_air_neutral(); },
                                         Actions::Type::Air_Neutral };
            }

            //========================================================//

            else if (state.move == State::Move::Falling)
            {

            }
        }
    }
}

//============================================================================//

void Fighter::impl_update_fighter()
{
    //========================================================//

    // apply friction /////

    if (state.move == State::Move::None || state.move == State::Move::Walking || state.move == State::Move::Dashing)
    {
        if (mVelocity.x < -0.f) mVelocity.x = maths::min(mVelocity.x + (stats.land_traction * 0.5f), -0.f);
        if (mVelocity.x > +0.f) mVelocity.x = maths::max(mVelocity.x - (stats.land_traction * 0.5f), +0.f);
    }

    if (state.move == State::Move::Jumping || state.move == State::Move::Falling)
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

    // update position /////

    mCurrentPosition += mVelocity / 48.f;

    //========================================================//

    // check landing /////

    if (state.move == State::Move::Jumping)
    {
        if (mCurrentPosition.y <= 0.f)
        {
            mVelocity.y = mCurrentPosition.y = 0.f;
            state.move = State::Move::None;
        }
    }

    //========================================================//

    // update active action /////

    if (actions->active.func != nullptr && actions->active.func())
        actions->active = { nullptr, Actions::Type::None };
}

//============================================================================//

void Fighter::base_tick_fighter()
{
    const auto input = controller.get_input();

    this->impl_validate_stats();

    this->impl_input_movement(input);
    this->impl_input_actions(input);

    this->impl_update_fighter();
}

//============================================================================//

void Fighter::impl_validate_stats()
{
    SQASSERT(stats.walk_speed / stats.dash_speed < (8.f / 5.f), "invalid walk/dash ratio");
    SQASSERT(stats.hop_height / stats.jump_height < (3.f / 2.f), "invalid hop/jump ratio");
}
