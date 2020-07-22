#include "game/Fighter.hpp"

#include "game/Stage.hpp"

#include <sqee/macros.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>

namespace maths = sq::maths;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

void Fighter::state_transition(State newState, uint fadeNow, const Animation* animNow, uint fadeAfter, const Animation* animAfter)
{
    SQASSERT(fadeNow == 0u || animNow != nullptr, "");
    SQASSERT(fadeAfter == 0u || animAfter != nullptr, "");
    SQASSERT(newState == State::EditorPreview || animNow != nullptr || animAfter != nullptr, "");

    status.state = newState;
    mStateProgress = 0u;

    if (animNow != nullptr)
    {
        // check if animNow is an already playing loop animation
        const bool alreadyPlaying = mAnimation == animNow && mAnimation->anim.looping();

        if (alreadyPlaying == false)
        {
            mAnimation = animNow;
            mFadeFrames = fadeNow;

            mAnimTimeDiscrete = 0u;
            mAnimTimeContinuous = 0.f;

            mFadeProgress = 0u;
            mFadeStartPose = current.pose;
            mFadeStartRotation = current.rotation;

            mAnimChangeFacing = false;
        }
    }

    mNextAnimation = animAfter;
    mNextFadeFrames = fadeAfter;
}

//============================================================================//

void Fighter::switch_action(ActionType type)
{
    Action* const newAction = get_action(type);

    SQASSERT(newAction != mActiveAction, "switch to same action");

    const Animations& anims = mAnimations;

    //--------------------------------------------------------//

    SWITCH ( type ) {

    //--------------------------------------------------------//

    CASE (NeutralFirst) state_transition(State::Action, 1u, &anims.NeutralFirst, 0u, nullptr);

    CASE (TiltDown)    state_transition(State::Action, 1u, &anims.TiltDown, 0u, nullptr);
    CASE (TiltForward) state_transition(State::Action, 1u, &anims.TiltForward, 0u, nullptr);
    CASE (TiltUp)      state_transition(State::Action, 1u, &anims.TiltUp, 0u, nullptr);

    CASE (AirBack)    state_transition(State::AirAction, 1u, &anims.AirBack, 0u, nullptr);
    CASE (AirDown)    state_transition(State::AirAction, 1u, &anims.AirDown, 0u, nullptr);
    CASE (AirForward) state_transition(State::AirAction, 1u, &anims.AirForward, 0u, nullptr);
    CASE (AirNeutral) state_transition(State::AirAction, 1u, &anims.AirNeutral, 0u, nullptr);
    CASE (AirUp)      state_transition(State::AirAction, 1u, &anims.AirUp, 0u, nullptr);

    CASE (DashAttack) state_transition(State::Action, 1u, &anims.DashAttack, 0u, nullptr);

    CASE (SmashDown)    state_transition(State::Charge, 1u, &anims.SmashDownStart, 0u, &anims.SmashDownCharge);
    CASE (SmashForward) state_transition(State::Charge, 1u, &anims.SmashForwardStart, 0u, &anims.SmashForwardCharge);
    CASE (SmashUp)      state_transition(State::Charge, 1u, &anims.SmashUpStart, 0u, &anims.SmashUpCharge);

    CASE (SpecialDown)    {} // todo
    CASE (SpecialForward) {} // todo
    CASE (SpecialNeutral) {} // todo
    CASE (SpecialUp)      {} // todo

    CASE (EvadeBack)    state_transition(State::Action, 1u, &anims.EvadeBack, 0u, nullptr);
    CASE (EvadeForward) state_transition(State::Action, 1u, &anims.EvadeForward, 0u, nullptr);
    CASE (Dodge)        state_transition(State::Action, 1u, &anims.Dodge, 0u, nullptr);

    CASE (AirDodge) state_transition(State::AirAction, 1u, &anims.AirDodge, 0u, nullptr);

    //--------------------------------------------------------//

    CASE ( None )
    {
        // we know here that current action is not nullptr

        SWITCH ( mActiveAction->get_type() ) {

        CASE ( NeutralFirst, TiltForward, TiltUp, SmashDown, SmashForward, SmashUp, DashAttack )
        state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop );

        CASE ( TiltDown )
        state_transition(State::Crouch, 0u, nullptr, 0u, &anims.CrouchLoop );

        CASE ( AirBack, AirDown, AirForward, AirNeutral, AirUp )
        state_transition(State::Jumping, 0u, nullptr, 0u, &anims.FallingLoop );

        CASE ( SpecialDown, SpecialForward, SpecialNeutral, SpecialUp )
        state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop ); // todo

        CASE ( EvadeBack, EvadeForward, Dodge )
        state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop );

        CASE ( AirDodge )
        state_transition(State::Jumping, 0u, nullptr, 0u, &anims.FallingLoop ); // todo

        CASE ( None ) SQASSERT(false, "switch from None to None");

        } SWITCH_END;
    }

    //--------------------------------------------------------//

    } SWITCH_END;

    //--------------------------------------------------------//

    if (mActiveAction != nullptr)
        mActiveAction->do_cancel();

    mActiveAction = newAction;

    if (mActiveAction != nullptr && status.state != State::Charge)
        mActiveAction->do_start();
}

//============================================================================//

void Fighter::update_commands(const Controller::Input& input)
{
    for (int i = STS_CMD_BUFFER_SIZE - 1u; i != 0; --i)
        mCommands[i] = std::move(mCommands[i-1]);

    mCommands[0].clear();

    Vector<Command>& cmds = mCommands[0];

    //--------------------------------------------------------//

    if (input.press_shield == true)
        cmds.push_back(Command::Shield);

    if (input.press_jump == true)
        cmds.push_back(Command::Jump);

    if (status.facing == +1 && input.norm_axis.x == -1)
        cmds.push_back(Command::TurnLeft);

    if (status.facing == -1 && input.norm_axis.x == +1)
        cmds.push_back(Command::TurnRight);

    if (input.mash_axis.y == -1)
        cmds.push_back(Command::MashDown);

    if (input.mash_axis.y == +1)
        cmds.push_back(Command::MashUp);

    if (input.mash_axis.x == -1)
        cmds.push_back(Command::MashLeft);

    if (input.mash_axis.x == +1)
        cmds.push_back(Command::MashRight);

    if (input.press_attack == true)
    {
        if      (input.mod_axis.y == -1) cmds.push_back(Command::SmashDown);
        else if (input.mod_axis.y == +1) cmds.push_back(Command::SmashUp);
        else if (input.mod_axis.x == -1) cmds.push_back(Command::SmashLeft);
        else if (input.mod_axis.x == +1) cmds.push_back(Command::SmashRight);
        else if (input.int_axis.y == -2) cmds.push_back(Command::AttackDown);
        else if (input.int_axis.y == +2) cmds.push_back(Command::AttackUp);
        else if (input.int_axis.x == -2) cmds.push_back(Command::AttackLeft);
        else if (input.int_axis.x == +2) cmds.push_back(Command::AttackRight);
        else if (input.int_axis.y == -1) cmds.push_back(Command::AttackDown);
        else if (input.int_axis.y == +1) cmds.push_back(Command::AttackUp);
        else if (input.int_axis.x == -1) cmds.push_back(Command::AttackLeft);
        else if (input.int_axis.x == +1) cmds.push_back(Command::AttackRight);
        else    cmds.push_back(Command::AttackNeutral);
    }
}

//============================================================================//

void Fighter::update_transitions(const Controller::Input& input)
{
    Stage& stage = world.get_stage();

    const Animations& anims = mAnimations;

    //--------------------------------------------------------//

    const auto try_catch_ledge = [&]() -> bool
    {
        SQASSERT(status.ledge == nullptr, "");

        if (mTimeSinceLedge <= STS_NO_LEDGE_CATCH_TIME)
            return false;

        // todo: unsure if position should be fighter's origin, or the centre of it's diamond
        status.ledge = stage.find_ledge(current.position, input.norm_axis.x);

        if (status.ledge == nullptr)
            return false;

        mTimeSinceLedge = 0u;

        // steal the ledge from some other fighter
        if (status.ledge->grabber != nullptr)
            status.ledge->grabber->status.ledge = nullptr;

        status.ledge->grabber = this;
        status.facing = -status.ledge->direction;

        return true;
    };

    //--------------------------------------------------------//

    const auto state_transition_prejump = [&]()
    {
        state_transition(State::PreJump, 1u, &anims.PreJump, 0u, nullptr);

        mExtraJumps = stats.extra_jumps;
        mJumpHeld = true;
    };

    const auto do_animated_facing_change = [&]()
    {
        mAnimChangeFacing = true;
        status.facing = -status.facing;
    };

    //--------------------------------------------------------//

    // This is the heart of the fighter state machine. Each tick this should do
    // either zero or one state transition, not more. Things not coinciding with
    // these transitions should be done elsewhere.

    SWITCH ( status.state ) {

    CASE ( Neutral ) //=======================================//
    {
        if (consume_command(Command::Shield))
            state_transition(State::Shield, 2u, &anims.ShieldOn, 2u, &anims.ShieldLoop);

        else if (consume_command(Command::Jump))
            state_transition_prejump();

        else if (consume_command(Command::SmashDown))
            switch_action(ActionType::SmashDown);

        else if (consume_command(Command::SmashUp))
            switch_action(ActionType::SmashUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            switch_action(ActionType::SmashForward);

        else if (consume_command(Command::AttackDown))
            switch_action(ActionType::TiltDown);

        else if (consume_command(Command::AttackUp))
            switch_action(ActionType::TiltUp);

        else if (consume_command_facing(Command::AttackLeft, Command::AttackRight))
            switch_action(ActionType::TiltForward);

        else if (consume_command(Command::AttackNeutral))
            switch_action(ActionType::NeutralFirst);

        else if (consume_command(Command::MashDown))
            current.position.y -= 0.1f;

        else if (consume_command_facing(Command::TurnRight, Command::TurnLeft))
        {
            state_transition(State::Neutral, 0u, &anims.Turn, 0u, &anims.NeutralLoop);
            do_animated_facing_change();
        }

        else if (input.norm_axis.x == status.facing)
        {
            if (mAnimation == &anims.Turn)
                state_transition(State::Walking, 0u, nullptr, 4u, &anims.WalkingLoop);
            else if (mVertigoActive == false)
                state_transition(State::Walking, 4u, &anims.WalkingLoop, 0u, nullptr);
            else status.state = State::Walking;
        }

        else if (input.int_axis.y == -2)
            state_transition(State::Crouch, 2u, &anims.CrouchOn, 0u, &anims.CrouchLoop);
    }

    CASE ( Walking ) //=======================================//
    {
        if (consume_command(Command::Shield))
            state_transition(State::Shield, 2u, &anims.ShieldOn, 0u, &anims.ShieldLoop);

        else if (consume_command(Command::Jump))
            state_transition_prejump();

        else if (consume_command(Command::SmashDown))
            switch_action(ActionType::SmashDown);

        else if (consume_command(Command::SmashUp))
            switch_action(ActionType::SmashUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            switch_action(ActionType::SmashForward);

        else if (consume_command(Command::AttackDown))
            switch_action(ActionType::TiltDown);

        else if (consume_command(Command::AttackUp))
            switch_action(ActionType::TiltUp);

        else if (consume_command_facing(Command::AttackLeft, Command::AttackRight))
            switch_action(ActionType::TiltForward);

        else if (consume_command_facing(Command::MashLeft, Command::MashRight))
            state_transition(State::Dashing, 0u, &anims.DashStart, 0u, nullptr);

        else if (input.int_axis.x == 0)
        {
            if (mAnimation == &anims.Turn)
                state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
            else if (mVertigoActive == false)
                state_transition(State::Neutral, 4u, &anims.NeutralLoop, 0u, nullptr);
            else status.state = State::Neutral;
        }

        else if (input.int_axis.y == -2)
            state_transition(State::Crouch, 2u, &anims.CrouchOn, 0u, &anims.CrouchLoop);
    }

    CASE ( Dashing ) //=======================================//
    {
        // currently, dash start is the same state and speed as dashing
        // this may change in the future to better match smash bros
        //
        // also, currently not designed with right stick smashes in mind, so
        // dashing then doing a back smash without braking first won't work

        if (consume_command(Command::Shield))
            state_transition(State::Shield, 2u, &anims.ShieldOn, 0u, &anims.ShieldLoop);

        else if (consume_command(Command::Jump))
            state_transition_prejump();

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            switch_action(ActionType::SmashForward);

        else if (consume_command_facing(Command::AttackLeft, Command::AttackRight))
            switch_action(ActionType::DashAttack);

        else if (input.int_axis.x != status.facing * 2)
        {
            if (mAnimation == &anims.DashStart)
                state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
            else
                state_transition(State::Brake, 4u, &anims.Brake, 0u, nullptr);
        }

        // this shouldn't need a fade, but mario's animation doesn't line up
        else if (mAnimation == &anims.DashStart && mStateProgress == stats.dash_start_time)
            state_transition(State::Dashing, 2u, &anims.DashingLoop, 0u, nullptr);
    }

    CASE ( Brake ) //=========================================//
    {
        // why can't we shield while braking? needs testing

        if (consume_command(Command::Jump))
            state_transition_prejump();

        else if (consume_command(Command::SmashDown))
            switch_action(ActionType::SmashDown);

        else if (consume_command(Command::SmashUp))
            switch_action(ActionType::SmashUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            switch_action(ActionType::SmashForward);

        else if (consume_command_oldest({Command::AttackDown, Command::AttackUp, Command::AttackLeft,
                                         Command::AttackRight, Command::AttackNeutral}))
            switch_action(ActionType::DashAttack);

        else if (mAnimation == &anims.Brake)
        {
            if (consume_command_facing(Command::TurnRight, Command::TurnLeft))
            {
                state_transition(State::Brake, 0u, &anims.TurnDash, 0u, nullptr);
                do_animated_facing_change();
            }

            else if (mStateProgress == stats.dash_brake_time)
                state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
        }

        else if (mStateProgress == stats.dash_turn_time)
        {
            SQASSERT(mAnimation == &anims.TurnDash, "");

            if (input.int_axis.x == status.facing * 2)
                state_transition(State::Dashing, 0u, nullptr, 0u, &anims.DashingLoop);
            else
                state_transition(State::Neutral, 0u, &anims.TurnBrake, 0u, &anims.NeutralLoop);
        }
    }

    CASE ( Crouch ) //========================================//
    {
        // we only handle down smashes and tilts in the crouch state
        // inputing any other attack will also cause the crouch state to
        // end, so the new state can do the attack next frame

        if (consume_command(Command::Shield))
            state_transition(State::Shield, 2u, &anims.ShieldOn, 0u, &anims.ShieldLoop);

        else if (consume_command(Command::Jump))
            state_transition_prejump();

        else if (consume_command(Command::SmashDown))
            switch_action(ActionType::SmashDown);

        else if (consume_command(Command::AttackDown))
            switch_action(ActionType::TiltDown);

        else if (input.int_axis.y != -2)
            state_transition(State::Neutral, 2u, &anims.CrouchOff, 0u, &anims.NeutralLoop);
    }

    CASE ( PreJump ) //=======================================//
    {
        if (mStateProgress == STS_JUMP_DELAY)
        {
            if (input.norm_axis.x == -status.facing)
                state_transition(State::Jumping, 1u, &anims.JumpBack, 0u, &anims.FallingLoop);
            else
                state_transition(State::Jumping, 1u, &anims.JumpForward, 0u, &anims.FallingLoop);

            // this could be a seperate stat, but half air speed seems good enough
            status.velocity.x = stats.air_speed * input.float_axis.x * 0.5f;

            const float height = mJumpHeld ? stats.jump_height : stats.hop_height;
            status.velocity.y = std::sqrt(2.f * height * stats.gravity) + stats.gravity * 0.5f;
        }
    }

    CASE ( Landing ) //=======================================//
    {
        if (mStateProgress == mLandingLag)
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
    }

    CASE ( Jumping ) //=======================================//
    {
        if (try_catch_ledge() == true)
            state_transition(State::LedgeHang, 1u, &anims.LedgeCatch, 0u, &anims.LedgeLoop);

        else if (consume_command(Command::Shield))
            switch_action(ActionType::AirDodge);

        else if (mExtraJumps > 0u && consume_command(Command::Jump))
        {
            if (input.norm_axis.x == -status.facing)
                state_transition(State::Jumping, 2u, &anims.AirHopBack, 1u, &anims.FallingLoop);
            else
                state_transition(State::Jumping, 2u, &anims.AirHopForward, 1u, &anims.FallingLoop);

            mExtraJumps -= 1u;
            status.velocity.y = std::sqrt(2.f * stats.gravity * stats.airhop_height) + stats.gravity * 0.5f;
        }

        else if (consume_command_oldest({Command::SmashDown, Command::AttackDown}))
            switch_action(ActionType::AirDown);

        else if (consume_command_oldest({Command::SmashUp, Command::AttackUp}))
            switch_action(ActionType::AirUp);

        else if (consume_command_oldest_facing({Command::SmashLeft, Command::AttackLeft}, {Command::SmashRight, Command::AttackRight}))
            switch_action(ActionType::AirForward);

        else if (consume_command_oldest_facing({Command::SmashRight, Command::AttackRight}, {Command::SmashLeft, Command::AttackLeft}))
            switch_action(ActionType::AirBack);

        else if (consume_command(Command::AttackNeutral))
            switch_action(ActionType::AirNeutral);
    }

    CASE ( Shield ) //========================================//
    {
        if (consume_command(Command::Jump))
            state_transition_prejump();

        else if (consume_command_facing(Command::MashLeft, Command::MashRight))
        {
            switch_action(ActionType::EvadeForward);
            do_animated_facing_change();
        }

        else if (consume_command_facing(Command::MashRight, Command::MashLeft))
            switch_action(ActionType::EvadeBack);

        else if (consume_command_oldest({Command::MashDown, Command::MashUp}))
            switch_action(ActionType::Dodge);

        else if (input.hold_shield == false)
            state_transition(State::Neutral, 2u, &anims.ShieldOff, 0u, &anims.NeutralLoop);
    }

    CASE ( LedgeHang ) //=====================================//
    {
        // someone else stole our ledge on the previous frame
        if (status.ledge == nullptr)
        {
            // todo: this needs to be handled later, to give p2 a chance to steal p1's ledge
            state_transition(State::Jumping, 0u, &anims.FallingLoop, 0u, nullptr);
            mTranslate.x -= mLocalDiamond.halfWidth * float(status.facing);
            mTranslate.y -= mLocalDiamond.offsetTop * 0.75f;
            mExtraJumps = stats.extra_jumps;
        }

        else if (mStateProgress >= STS_MIN_LEDGE_HANG_TIME)
        {
            if (consume_command(Command::Jump))
            {
                state_transition(State::Jumping, 1u, &anims.LedgeJump, 0u, &anims.FallingLoop);
                status.velocity.y = std::sqrt(2.f * stats.jump_height * stats.gravity) + stats.gravity * 0.5f;
                status.ledge->grabber = nullptr;
                status.ledge = nullptr;
                mExtraJumps = stats.extra_jumps;
            }

            else if (consume_command(Command::MashUp) ||
                     consume_command_facing(Command::MashLeft, Command::MashRight))
            {
                state_transition(State::LedgeClimb, 1u, &anims.LedgeClimb, 0u, nullptr);
                status.ledge->grabber = nullptr;
                status.ledge = nullptr;
            }

            else if (consume_command(Command::MashDown) ||
                     consume_command_facing(Command::MashRight, Command::MashLeft))
            {
                state_transition(State::Jumping, 0u, &anims.FallingLoop, 0u, nullptr);
                status.ledge->grabber = nullptr;
                status.ledge = nullptr;
                mTranslate.x -= mLocalDiamond.halfWidth * float(status.facing);
                mTranslate.y -= mLocalDiamond.offsetTop * 0.75f;
                mExtraJumps = stats.extra_jumps;
            }
        }
    }

    CASE ( LedgeClimb ) //====================================//
    {
        if (mStateProgress == stats.ledge_climb_time)
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
    }

    CASE ( Charge ) //========================================//
    {
        SQASSERT(mActiveAction != nullptr, "can't charge null ptrs");

        if (input.hold_attack == false) // todo: or max charge reached
        {
            if (mActiveAction->get_type() == ActionType::SmashDown)
                state_transition(State::Action, 1u, &anims.SmashDownAttack, 0u, nullptr);

            else if (mActiveAction->get_type() == ActionType::SmashForward)
                state_transition(State::Action, 1u, &anims.SmashForwardAttack, 0u, nullptr);

            else if (mActiveAction->get_type() == ActionType::SmashUp)
                state_transition(State::Action, 1u, &anims.SmashUpAttack, 0u, nullptr);

            else SQASSERT(false, "only smash attacks can be charged");

            mActiveAction->do_start();
        }
    }

    //== Nothing to do here ==================================//

    CASE ( Action, AirAction ) {}

    CASE ( EditorPreview ) {}

    //== Not Yet Implemented =================================//

    CASE ( Helpless, Knocked, Stunned ) {}

    //--------------------------------------------------------//

    } SWITCH_END;
}

//============================================================================//

void Fighter::update_states(const Controller::Input& input)
{
    Stage& stage = world.get_stage();

    const Animations& anims = mAnimations;

    mStateProgress += 1u;

    //-- misc non-transition state updates -------------------//

    if (status.state == State::PreJump)
        if (input.hold_jump == false)
            mJumpHeld = false;

    //-- most updates don't apply when ledge hanging ---------//

    if (status.state == State::LedgeHang)
    {
        SQASSERT(status.ledge != nullptr, "nope");

        // this will be the case only if we just grabbed the ledge
        if (status.velocity != Vec2F())
        {
            current.position = (current.position + status.ledge->position) / 2.f;
            status.velocity = Vec2F();
        }
        else current.position = status.ledge->position;

        return; // EARLY RETURN
    }

    mTimeSinceLedge += 1u;

    //-- apply friction --------------------------------------//

    SWITCH ( status.state ) {

    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Charge, Action, Landing, Shield )
    {
        if (status.velocity.x < -0.f) status.velocity.x = maths::min(status.velocity.x + stats.traction, -0.f);
        if (status.velocity.x > +0.f) status.velocity.x = maths::max(status.velocity.x - stats.traction, +0.f);
    }

    CASE ( Jumping, Helpless, AirAction )
    {
        if (input.int_axis.x == 0)
        {
            if (status.velocity.x < -0.f) status.velocity.x = maths::min(status.velocity.x + stats.air_friction, -0.f);
            if (status.velocity.x > +0.f) status.velocity.x = maths::max(status.velocity.x - stats.air_friction, +0.f);
        }
    }

    CASE ( PreJump, Knocked, Stunned, LedgeHang, LedgeClimb, EditorPreview ) {}

    } SWITCH_END;

    //-- add horizontal velocity -----------------------------//

    const auto apply_horizontal_move = [&](float mobility, float speed)
    {
        if (input.int_axis.x < 0 && status.velocity.x <= input.float_axis.x * speed) return;
        if (input.int_axis.x > 0 && status.velocity.x >= input.float_axis.x * speed) return;

        status.velocity.x += float(input.norm_axis.x) * mobility;
        status.velocity.x = maths::clamp_magnitude(status.velocity.x, input.float_axis.x * speed);
    };

    if (input.int_axis.x != 0)
    {
        if (status.state == State::Jumping || status.state == State::Helpless)
            apply_horizontal_move(stats.air_mobility, stats.air_speed);

        else if (status.state == State::AirAction)
            apply_horizontal_move(stats.air_mobility, stats.air_speed);

        else if (status.state == State::Walking)
            apply_horizontal_move(stats.traction * 2.f, stats.walk_speed);

        else if (status.state == State::Dashing)
            apply_horizontal_move(stats.traction * 4.f, stats.dash_speed);
    }

    //-- apply gravity, velocity, and translation ------------//

    status.velocity.y = maths::max(status.velocity.y - stats.gravity, -stats.fall_speed);

    const Vec2F translation = status.velocity + mTranslate;
    const Vec2F targetPosition = current.position + translation;

    //-- ask the stage where we can move ---------------------//

    const bool edgeStop = ( status.state == State::Neutral || status.state == State::Shield || status.state == State::Landing ||
                            status.state == State::PreJump || status.state == State::Action || status.state == State::Charge ) ||
                          ( ( status.state == State::Walking || status.state == State::Dashing || status.state == State::Brake ) &&
                            !( input.int_axis.x == -2 && translation.x < -0.0f ) &&
                            !( input.int_axis.x == +2 && translation.x > +0.0f ) );

    MoveAttempt moveAttempt = stage.attempt_move(mLocalDiamond, current.position, targetPosition, edgeStop);

    current.position = moveAttempt.result;

    mTranslate = Vec2F();

    //-- activate vertigo animation --------------------------//

    if (status.state == State::Neutral || status.state == State::Walking)
    {
        if (mVertigoActive == false && moveAttempt.edge == status.facing)
        {
            state_transition(status.state, 2u, &anims.VertigoStart, 0u, &anims.VertigoLoop);
            mVertigoActive = true;
        }
    }
    else mVertigoActive = false;

    //-- handle starting to fall and landing -----------------//

    SWITCH (status.state) {

    // normal air states, can do light or heavy landing
    CASE ( Jumping, Helpless )
    {
        if (moveAttempt.collideFloor == true)
        {
            const bool light = status.velocity.y > -stats.fall_speed || mStateProgress < stats.land_heavy_min_time;

            state_transition(State::Landing, 1u, light ? &anims.LandLight : &anims.LandHeavy, 0u, nullptr);
            mLandingLag = light ? STS_LIGHT_LANDING_LAG : STS_HEAVY_LANDING_LAG;

            status.velocity.y = 0.f;
        }
    }

    // air actions all have different amounts of landing lag
    CASE ( AirAction )
    {
        if (moveAttempt.collideFloor == true)
        {
            switch_action(ActionType::None);

            state_transition(State::Landing, 1u, &anims.LandHeavy, 0u, nullptr);
            mLandingLag = STS_HEAVY_LANDING_LAG * 2u; // todo

            status.velocity.y = 0.f;
        }
    }

    // landing while knocked causes you to bounce and be stuck for a bit
    CASE ( Knocked )
    {
        if (moveAttempt.collideFloor == true)
        {
            if (maths::length(status.velocity) > 5.f)
            {
                // todo
            }
            state_transition(State::Landing, 1u, &anims.LandHeavy, 0u, nullptr);
            mLandingLag = STS_HEAVY_LANDING_LAG;

            status.velocity.y = 0.f;
        }
    }

    // these ground states have a special dive animation
    CASE ( Walking, Dashing )
    {
        if (moveAttempt.collideFloor == false)
        {
            // todo
            state_transition(State::Jumping, 1u, &anims.FallingLoop, 0u, nullptr);

            mExtraJumps = stats.extra_jumps;
        }
        else status.velocity.y = 0.f;
    }

    // misc ground states that you can fall from
    CASE ( Neutral, Action, Brake, Crouch, Charge, Landing, Stunned, Shield )
    {
        if (moveAttempt.collideFloor == false)
        {
            state_transition(State::Jumping, 2u, &anims.FallingLoop, 0u, nullptr);

            mExtraJumps = stats.extra_jumps; // todo: is this correct for Action?
        }
        else status.velocity.y = 0.f;
    }

    // special states that you can't fall or land from
    CASE ( PreJump, LedgeHang, LedgeClimb, EditorPreview ) {}

    } SWITCH_END;

    //-- check if we've hit a ceiling or wall ----------------//

    // todo: this should cause our fighter to bounce
    if (status.state == State::Knocked)
    {
        if (moveAttempt.collideCeiling == true)
            status.velocity.y = 0.f;

        if (moveAttempt.collideWall == true)
            status.velocity.x = 0.f;
    }

    // todo: this should probably have some animations
    else
    {
        if (moveAttempt.collideCeiling == true)
            status.velocity.y = 0.f;

        if (moveAttempt.collideWall == true)
            status.velocity.x = 0.f;
    }

    //-- update the active action ----------------------------//

    if (mActiveAction != nullptr && status.state != State::Charge)
        if (mActiveAction->do_tick() == true)
            switch_action(ActionType::None);
}

//============================================================================//

void Fighter::update_animation()
{
    // this shouldn't happen if all anims are correct, but might happen if for example
    // attack anims are too short, like when they don't exist and a null anim is used
    if (mAnimation == nullptr && mNextAnimation != nullptr)
    {
        mAnimation = mNextAnimation;
        mNextAnimation = nullptr;
    }

    //--------------------------------------------------------//

    // todo: calculate this once when we load the armature
    const QuatF rootInverseRestRotation = maths::inverse(mArmature.get_rest_pose().front().rotation);

    // helper to get the current pose of the root bone
    const auto root = [&]() -> sq::Armature::Bone& { return current.pose.front(); };

    // called when mAnimation finishes for non-looping animations
    const auto finish_animation = [&]()
    {
        mAnimation = mNextAnimation;
        mNextAnimation = nullptr;

        mAnimTimeDiscrete = 0u;
        mAnimTimeContinuous = 0.f;

        mFadeProgress = 0u;

        mFadeStartPose = current.pose;
        mFadeStartRotation = current.rotation;

        mAnimChangeFacing = false;
    };

    //--------------------------------------------------------//

    const auto set_current_pose_discrete = [&](const Animation& anim, uint time)
    {
        current.pose = mArmature.compute_pose_discrete(anim.anim, time);
        current.rotation = root().rotation * rootInverseRestRotation * current.rotation;
        root().rotation = mArmature.get_rest_pose().front().rotation;

        sq::log_info("pose: %s - %d", anim.key, time);

        mPreviousAnimation = &anim;
        mPreviousAnimTimeDiscrete = time;
    };

    const auto set_current_pose_continuous = [&](const Animation& anim, float time)
    {
        current.pose = mArmature.compute_pose_continuous(anim.anim, time);
        current.rotation = root().rotation * rootInverseRestRotation * current.rotation;
        root().rotation = mArmature.get_rest_pose().front().rotation;

        sq::log_info("pose: %s - %.4f", anim.key, time);

        mPreviousAnimation = &anim;
        mPreviousAnimTimeContinuous = time;
    };

    //--------------------------------------------------------//

    if (mAnimation != nullptr)
    {
        SWITCH (mAnimation->mode) {

        CASE (Standard) //========================================//
        {
            SQASSERT(mAnimation->anim.looping() == false, "");

            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            if (++mAnimTimeDiscrete == mAnimation->anim.totalTime)
                finish_animation();
        }

        CASE (Looping) //=========================================//
        {
            SQASSERT(mAnimation->anim.looping() == true, "");
            SQASSERT(mNextAnimation == nullptr, "");

            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            if (++mAnimTimeDiscrete == mAnimation->anim.totalTime)
                mAnimTimeDiscrete = 0u;
        }

        CASE (WalkCycle) //=======================================//
        {
            SQASSERT(mAnimation->anim.looping() == true, "");
            SQASSERT(mNextAnimation == nullptr, "");

            const float distance = std::abs(status.velocity.x);
            const float animSpeed = stats.anim_walk_stride / float(mAnimation->anim.totalTime);

            mAnimTimeContinuous += distance / animSpeed;

            set_current_pose_continuous(*mAnimation, mAnimTimeContinuous);

            root().offset = Vec3F();
        }

        CASE (DashCycle) //=======================================//
        {
            SQASSERT(mAnimation->anim.looping() == true, "");
            SQASSERT(mNextAnimation == nullptr, "");

            const float distance = std::abs(status.velocity.x);
            const float length = stats.anim_dash_stride / float(mAnimation->anim.totalTime);

            mAnimTimeContinuous += distance / length;

            set_current_pose_continuous(*mAnimation, mAnimTimeContinuous);

            root().offset = Vec3F();
        }

        CASE (ApplyMotion) //=====================================//
        {
            SQASSERT(mAnimation->anim.looping() == false, "");

            if (mAnimTimeDiscrete == 0u)
                mPrevRootMotionOffset = Vec3F();

            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            const Vec3F offsetLocal = root().offset - mPrevRootMotionOffset;

            mTranslate = { mAnimChangeFacing ? -offsetLocal.z : offsetLocal.z, offsetLocal.x };
            mTranslate.x *= float(status.facing);

            mPrevRootMotionOffset = root().offset;
            root().offset = offsetLocal;

            if (++mAnimTimeDiscrete == mAnimation->anim.totalTime)
                finish_animation();
        }

        } SWITCH_END;
    }

    //-- blend from fade pose for smooth transitions ---------//

    if (mFadeProgress != mFadeFrames)
    {
        const float blend = float(++mFadeProgress) / float(mFadeFrames + 1u);
        current.pose = mArmature.blend_poses(mFadeStartPose, current.pose, blend);
        current.rotation = maths::slerp(mFadeStartRotation, current.rotation, blend);
        sq::log_info("blend - %d / %d - %.3f", mFadeProgress, mFadeFrames, blend);
    }
    else mFadeProgress = mFadeFrames = 0u;

    //-- if an animation just finished, update fadeFrames ----//

    if (mNextAnimation == nullptr && mNextFadeFrames != 0u)
    {
        mFadeFrames = mNextFadeFrames;
        mNextFadeFrames = 0u;
    }
}

//============================================================================//

void Fighter::base_tick_fighter()
{
    previous = current;

    //--------------------------------------------------------//

    if (world.globals.editorMode == true)
    {
        const auto input = Controller::Input();

        update_commands(input);
        update_transitions(input);
        update_states(input);
    }
    else
    {
        SQASSERT(mController != nullptr, "");
        const auto input = mController->get_input();

        update_commands(input);
        update_transitions(input);
        update_states(input);
    }

    //--------------------------------------------------------//

    const int8_t rotFacing = status.facing * (mAnimChangeFacing ? -1 : +1);
    const float rotY = status.state == State::EditorPreview ? 0.5f : 0.25f * float(rotFacing);

    current.rotation = QuatF(0.f, rotY, 0.f);

    update_animation();

    mArmature.compute_ubo_data(current.pose, mBoneMatrices.data(), uint(mBoneMatrices.size()));

    mModelMatrix = maths::transform(Vec3F(current.position, 0.f), current.rotation, Vec3F(1.f));
}
