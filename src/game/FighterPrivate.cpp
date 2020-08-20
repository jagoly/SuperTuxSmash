#include "game/Fighter.hpp" // IWYU pragma: associated

#include "main/Options.hpp"

#include "game/Action.hpp"
#include "game/Controller.hpp"
#include "game/FightWorld.hpp"
#include "game/Stage.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>

using namespace sts;

//============================================================================//

void Fighter::state_transition(State state, uint fadeNow, const Animation* animNow, uint fadeAfter, const Animation* animAfter)
{
    SQASSERT(fadeNow == 0u || animNow != nullptr, "");
    SQASSERT(fadeAfter == 0u || animAfter != nullptr, "");
    SQASSERT(animAfter == nullptr || animAfter->anim.looping(), "");

    status.state = state;
    mStateProgress = 0u;

    // current animation will be kept if animNow is null
    if (animNow != nullptr)
    {
        // don't restart already playing loop animations
        if (mAnimation != animNow || !mAnimation->anim.looping())
        {
            mAnimation = animNow;
            mFadeFrames = fadeNow;

            mAnimTimeDiscrete = 0u;
            mAnimTimeContinuous = 0.f;

            mFadeProgress = 0u;
            mFadeStartRotation = current.rotation;
            mFadeStartPose = current.pose;
        }
    }

    // next animation is never kept, if animAfter is null it will be cleared
    mNextAnimation = animAfter;
    mNextFadeFrames = fadeAfter;

    // don't set next animation if it's already playing
    if (mNextAnimation == mAnimation)
    {
        mNextAnimation = nullptr;
        mNextFadeFrames = 0u;
    }

}

//============================================================================//

void Fighter::state_transition_action(ActionType action)
{
    // if there is already an active action, cancel it
    cancel_active_action();

    const Animations& anims = mAnimations;

    SWITCH ( action ) {

    CASE (NeutralFirst) state_transition(State::Action, 1u, &anims.NeutralFirst, 0u, nullptr);

    CASE (DashAttack) state_transition(State::Action, 1u, &anims.DashAttack, 0u, nullptr);

    CASE (TiltDown)    state_transition(State::Action, 1u, &anims.TiltDown, 0u, nullptr);
    CASE (TiltForward) state_transition(State::Action, 1u, &anims.TiltForward, 0u, nullptr);
    CASE (TiltUp)      state_transition(State::Action, 1u, &anims.TiltUp, 0u, nullptr);

    CASE (EvadeBack)    state_transition(State::Action, 1u, &anims.EvadeBack, 0u, nullptr);
    CASE (EvadeForward) state_transition(State::Action, 1u, &anims.EvadeForward, 0u, nullptr);
    CASE (Dodge)        state_transition(State::Action, 1u, &anims.Dodge, 0u, nullptr);

    CASE (ProneAttack)  state_transition(State::Action, 1u, &anims.ProneAttack, 0u, nullptr);
    CASE (ProneBack)    state_transition(State::Action, 1u, &anims.ProneBack, 0u, nullptr);
    CASE (ProneForward) state_transition(State::Action, 1u, &anims.ProneForward, 0u, nullptr);
    CASE (ProneStand)   state_transition(State::Action, 1u, &anims.ProneStand, 0u, nullptr);

    CASE (LedgeClimb) state_transition(State::Action, 1u, &anims.LedgeClimb, 0u, nullptr);

    CASE (SmashDown)    state_transition(State::Charge, 1u, &anims.SmashDownStart, 0u, &anims.SmashDownCharge);
    CASE (SmashForward) state_transition(State::Charge, 1u, &anims.SmashForwardStart, 0u, &anims.SmashForwardCharge);
    CASE (SmashUp)      state_transition(State::Charge, 1u, &anims.SmashUpStart, 0u, &anims.SmashUpCharge);

    CASE (AirBack)    state_transition(State::AirAction, 1u, &anims.AirBack, 0u, nullptr);
    CASE (AirDown)    state_transition(State::AirAction, 1u, &anims.AirDown, 0u, nullptr);
    CASE (AirForward) state_transition(State::AirAction, 1u, &anims.AirForward, 0u, nullptr);
    CASE (AirNeutral) state_transition(State::AirAction, 1u, &anims.AirNeutral, 0u, nullptr);
    CASE (AirUp)      state_transition(State::AirAction, 1u, &anims.AirUp, 0u, nullptr);
    CASE (AirDodge)   state_transition(State::AirAction, 1u, &anims.AirDodge, 0u, nullptr);

    CASE (LandLight)  state_transition(State::Action, 1u, &anims.LandLight, 0u, nullptr);
    CASE (LandHeavy)  state_transition(State::Action, 1u, &anims.LandHeavy, 0u, nullptr);
    CASE (LandTumble) state_transition(State::Action, 1u, &anims.LandTumble, 0u, nullptr);

    CASE (LandAirBack)    state_transition(State::Action, 1u, &anims.LandAirBack, 0u, nullptr);
    CASE (LandAirDown)    state_transition(State::Action, 1u, &anims.LandAirDown, 0u, nullptr);
    CASE (LandAirForward) state_transition(State::Action, 1u, &anims.LandAirForward, 0u, nullptr);
    CASE (LandAirNeutral) state_transition(State::Action, 1u, &anims.LandAirNeutral, 0u, nullptr);
    CASE (LandAirUp)      state_transition(State::Action, 1u, &anims.LandAirUp, 0u, nullptr);

    CASE (SpecialDown)    {} // todo
    CASE (SpecialForward) {} // todo
    CASE (SpecialNeutral) {} // todo
    CASE (SpecialUp)      {} // todo

    CASE (None) SQASSERT(false, "can't switch to None");

    } SWITCH_END;

    mActiveAction = get_action(action);

    // if the action is not charging, start it now
    if (status.state != State::Charge) mActiveAction->do_start();
};

//============================================================================//

void Fighter::cancel_active_action()
{
    if (mActiveAction != nullptr)
    {
        // don't cancel an action that hasn't started yet
        if (mActiveAction->get_status() != ActionStatus::None)
            mActiveAction->do_cancel();

        mActiveAction = nullptr;
    }
}

//============================================================================//

void Fighter::update_commands(const InputFrame& input)
{
    for (size_t i = CMD_BUFFER_SIZE - 1u; i != 0u; --i)
        mCommands[i] = std::move(mCommands[i-1u]);

    mCommands.front().clear();

    std::vector<Command>& cmds = mCommands.front();

    //--------------------------------------------------------//

    const auto erase_command = [this](Command cmd)
    {
        for (auto& cmds : mCommands)
            if (auto iter = algo::find(cmds, cmd); iter != cmds.end())
                cmds.erase(iter);
    };

    if (status.facing == +1 && input.norm_axis.x == -1)
    {
        erase_command(Command::TurnLeft);
        cmds.push_back(Command::TurnLeft);
    }

    if (status.facing == -1 && input.norm_axis.x == +1)
    {
        erase_command(Command::TurnRight);
        cmds.push_back(Command::TurnRight);
    }

    //--------------------------------------------------------//

    if (input.press_shield == true)
        cmds.push_back(Command::Shield);

    if (input.press_jump == true)
        cmds.push_back(Command::Jump);

    if (input.mash_axis.y == -1)
        cmds.push_back(Command::MashDown);

    if (input.mash_axis.y == +1)
        cmds.push_back(Command::MashUp);

    if (input.mash_axis.x == -1)
        cmds.push_back(Command::MashLeft);

    if (input.mash_axis.x == +1)
        cmds.push_back(Command::MashRight);

    //--------------------------------------------------------//

    if (input.press_attack == true)
    {
        if      (input.mod_axis.y == -1) cmds.push_back(Command::SmashDown);
        else if (input.mod_axis.y == +1) cmds.push_back(Command::SmashUp);
        else if (input.mod_axis.x == -1) cmds.push_back(Command::SmashLeft);
        else if (input.mod_axis.x == +1) cmds.push_back(Command::SmashRight);
        else if (input.int_axis.y <= -1) cmds.push_back(Command::AttackDown);
        else if (input.int_axis.y >= +1) cmds.push_back(Command::AttackUp);
        else if (input.int_axis.x <= -1) cmds.push_back(Command::AttackLeft);
        else if (input.int_axis.x >= +1) cmds.push_back(Command::AttackRight);
        else    cmds.push_back(Command::AttackNeutral);
    }
}

//============================================================================//

void Fighter::update_action(const InputFrame& /*input*/)
{
    // Actions can override normal fighter behaviour, so they need to be updated first.
    // todo: actually doing this last so anims sync correctly, INVESTIGATE

    if (status.state == State::Charge) return;
    if (status.state == State::Freeze) return;
    if (mActiveAction == nullptr) return;

    mActiveAction->do_tick();

    //sq::log_debug("status: {}", mActiveAction->get_status());

    if (mActiveAction->get_status() == ActionStatus::Finished)
        mActiveAction = nullptr;

    else if (mActiveAction->get_status() == ActionStatus::RuntimeError)
        SQASSERT(world.options.editor_mode == true, "");
}

//============================================================================//

void Fighter::update_transitions(const InputFrame& input)
{
    // This is the heart of the fighter state machine. Each tick this should do
    // either zero or one state transition, not more. Things not coinciding with
    // these transitions should be done elsewhere.
    //
    // Transitions can be made elsewhere, but everything done in response to
    // input commands should be here, as well as whatever others are possible.

    Stage& stage = world.get_stage();

    const Animations& anims = mAnimations;

    //--------------------------------------------------------//

    const auto try_catch_ledge = [&]() -> bool
    {
        SQASSERT(status.ledge == nullptr, "");

        if (mTimeSinceLedge <= LEDGE_CATCH_NOPE_TIME)
            return false;

        // todo: unsure if position should be fighter's origin, or the centre of it's diamond
        status.ledge = stage.find_ledge(status.position, input.norm_axis.x);

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

    //--------------------------------------------------------//

    SWITCH ( status.state ) {

    CASE ( Neutral ) //=======================================//
    {
        if (consume_command(Command::Shield))
            state_transition(State::Shield, 2u, &anims.ShieldOn, 2u, &anims.ShieldLoop);

        else if (consume_command(Command::Jump))
            state_transition_prejump();

        else if (consume_command(Command::SmashDown))
            state_transition_action(ActionType::SmashDown);

        else if (consume_command(Command::SmashUp))
            state_transition_action(ActionType::SmashUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            state_transition_action(ActionType::SmashForward);

        else if (consume_command(Command::AttackDown))
            state_transition_action(ActionType::TiltDown);

        else if (consume_command(Command::AttackUp))
            state_transition_action(ActionType::TiltUp);

        else if (consume_command_facing(Command::AttackLeft, Command::AttackRight))
            state_transition_action(ActionType::TiltForward);

        else if (consume_command(Command::AttackNeutral))
            state_transition_action(ActionType::NeutralFirst);

        else if (consume_command(Command::MashDown))
            status.position.y -= 0.1f; // drop through ledge

        else if (consume_command_facing(Command::TurnRight, Command::TurnLeft))
        {
            state_transition(State::Neutral, 0u, &anims.Turn, 0u, &anims.NeutralLoop);
            status.facing = -status.facing;
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
            state_transition_action(ActionType::SmashDown);

        else if (consume_command(Command::SmashUp))
            state_transition_action(ActionType::SmashUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            state_transition_action(ActionType::SmashForward);

        else if (consume_command(Command::AttackDown))
            state_transition_action(ActionType::TiltDown);

        else if (consume_command(Command::AttackUp))
            state_transition_action(ActionType::TiltUp);

        else if (consume_command_facing(Command::AttackLeft, Command::AttackRight))
            state_transition_action(ActionType::TiltForward);

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
            state_transition_action(ActionType::SmashForward);

        else if (consume_command_oldest({Command::AttackDown, Command::AttackUp, Command::AttackLeft, Command::AttackRight}))
            state_transition_action(ActionType::DashAttack);

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
            state_transition_action(ActionType::SmashDown);

        else if (consume_command(Command::SmashUp))
            state_transition_action(ActionType::SmashUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            state_transition_action(ActionType::SmashForward);

        else if (consume_command_oldest({Command::AttackDown, Command::AttackUp, Command::AttackLeft,
                                         Command::AttackRight, Command::AttackNeutral}))
            state_transition_action(ActionType::DashAttack);

        else if (mAnimation == &anims.Brake)
        {
            if (consume_command_facing(Command::TurnRight, Command::TurnLeft))
            {
                state_transition(State::Brake, 0u, &anims.TurnDash, 0u, nullptr);
                status.facing = -status.facing;
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
            state_transition_action(ActionType::SmashDown);

        else if (consume_command(Command::AttackDown))
            state_transition_action(ActionType::TiltDown);

        else if (input.int_axis.y != -2)
            state_transition(State::Neutral, 2u, &anims.CrouchOff, 0u, &anims.NeutralLoop);
    }

    CASE ( PreJump ) //=======================================//
    {
        if (mStateProgress == JUMP_DELAY)
        {
            if (input.norm_axis.x == -status.facing)
                state_transition(State::JumpFall, 1u, &anims.JumpBack, 0u, &anims.FallingLoop);
            else
                state_transition(State::JumpFall, 1u, &anims.JumpForward, 0u, &anims.FallingLoop);

            // this could be a seperate stat, but half air speed seems good enough
            status.velocity.x = stats.air_speed * input.float_axis.x * 0.5f;

            const float height = mJumpHeld ? stats.jump_height : stats.hop_height;
            status.velocity.y = std::sqrt(2.f * height * stats.gravity) + stats.gravity * 0.5f;
        }
    }

    CASE ( Prone ) //=========================================//
    {
        if (consume_command_facing(Command::MashLeft, Command::MashRight))
            state_transition_action(ActionType::ProneForward);

        else if (consume_command_facing(Command::MashRight, Command::MashLeft))
            state_transition_action(ActionType::ProneBack);

        else if (consume_command_oldest({Command::MashUp, Command::Jump}))
            state_transition_action(ActionType::ProneStand);

        else if (consume_command_oldest({Command::AttackDown, Command::AttackUp, Command::AttackLeft,
                                         Command::AttackRight, Command::AttackNeutral}))
            state_transition_action(ActionType::ProneAttack);
    }

    CASE ( Shield ) //========================================//
    {
        if (consume_command(Command::Jump))
            state_transition_prejump();

        else if (consume_command_facing(Command::MashLeft, Command::MashRight))
        {
            state_transition_action(ActionType::EvadeForward);
            status.facing = -status.facing;
        }

        else if (consume_command_facing(Command::MashRight, Command::MashLeft))
            state_transition_action(ActionType::EvadeBack);

        else if (consume_command_oldest({Command::MashDown, Command::MashUp}))
            state_transition_action(ActionType::Dodge);

        else if (input.hold_shield == false)
            state_transition(State::Neutral, 2u, &anims.ShieldOff, 0u, &anims.NeutralLoop);
    }

    CASE ( JumpFall, TumbleFall ) //==========================//
    {
        if (try_catch_ledge() == true)
            state_transition(State::LedgeHang, 1u, &anims.LedgeCatch, 0u, &anims.LedgeLoop);

        else if (consume_command(Command::Shield))
            state_transition_action(ActionType::AirDodge);

        else if (mExtraJumps > 0u && consume_command(Command::Jump))
        {
            if (input.norm_axis.x == -status.facing)
                state_transition(State::JumpFall, 2u, &anims.AirHopBack, 1u, &anims.FallingLoop);
            else
                state_transition(State::JumpFall, 2u, &anims.AirHopForward, 1u, &anims.FallingLoop);

            mExtraJumps -= 1u;
            status.velocity.y = std::sqrt(2.f * stats.gravity * stats.airhop_height) + stats.gravity * 0.5f;
        }

        else if (consume_command_oldest({Command::SmashDown, Command::AttackDown}))
            state_transition_action(ActionType::AirDown);

        else if (consume_command_oldest({Command::SmashUp, Command::AttackUp}))
            state_transition_action(ActionType::AirUp);

        else if (consume_command_oldest_facing({Command::SmashLeft, Command::AttackLeft}, {Command::SmashRight, Command::AttackRight}))
            state_transition_action(ActionType::AirForward);

        else if (consume_command_oldest_facing({Command::SmashRight, Command::AttackRight}, {Command::SmashLeft, Command::AttackLeft}))
            state_transition_action(ActionType::AirBack);

        else if (consume_command(Command::AttackNeutral))
            state_transition_action(ActionType::AirNeutral);
    }

    CASE ( Stun ) //==========================================//
    {
        if (mStateProgress == mHitStunTime)
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
    }

    CASE ( AirStun ) //=======================================//
    {
        if (mStateProgress == mHitStunTime)
            state_transition(State::JumpFall, 0u, nullptr, 4u, &anims.FallingLoop);
    }

    CASE ( TumbleStun ) //====================================//
    {
        if (mStateProgress == mHitStunTime)
            state_transition(State::TumbleFall, 0u, nullptr, 0u, &anims.TumbleLoop);
    }

    CASE ( Helpless ) //======================================//
    {
        // todo
    }

    CASE ( LedgeHang ) //=====================================//
    {
        // someone else stole our ledge on the previous frame
        if (status.ledge == nullptr)
        {
            // todo: this needs to be handled later, to give p2 a chance to steal p1's ledge
            state_transition(State::JumpFall, 0u, &anims.FallingLoop, 0u, nullptr);
            mTranslate.x -= mLocalDiamond.halfWidth * float(status.facing);
            mTranslate.y -= mLocalDiamond.offsetTop * 0.75f;
            mExtraJumps = stats.extra_jumps;
        }

        else if (mStateProgress >= LEDGE_HANG_MIN_TIME)
        {
            if (consume_command(Command::Jump))
            {
                state_transition(State::JumpFall, 1u, &anims.LedgeJump, 0u, &anims.FallingLoop);
                status.velocity.y = std::sqrt(2.f * stats.jump_height * stats.gravity) + stats.gravity * 0.5f;
                status.ledge->grabber = nullptr;
                status.ledge = nullptr;
                mExtraJumps = stats.extra_jumps;
            }

            else if (consume_command(Command::MashUp) ||
                     consume_command_facing(Command::MashLeft, Command::MashRight))
            {
                state_transition_action(ActionType::LedgeClimb);
                status.ledge->grabber = nullptr;
                status.ledge = nullptr;
            }

            else if (consume_command(Command::MashDown) ||
                     consume_command_facing(Command::MashRight, Command::MashLeft))
            {
                state_transition(State::JumpFall, 0u, &anims.FallingLoop, 0u, nullptr);
                status.ledge->grabber = nullptr;
                status.ledge = nullptr;
                mTranslate.x -= mLocalDiamond.halfWidth * float(status.facing);
                mTranslate.y -= mLocalDiamond.offsetTop * 0.75f;
                mExtraJumps = stats.extra_jumps;
            }
        }
    }

    CASE ( Charge ) //========================================//
    {
        SQASSERT(mActiveAction != nullptr, "no active action for state 'Charge'");

        if (input.hold_attack == false) // todo: or max charge reached
        {
            if (mActiveAction->type == ActionType::SmashDown)
                state_transition(State::Action, 1u, &anims.SmashDownAttack, 0u, nullptr);

            else if (mActiveAction->type == ActionType::SmashForward)
                state_transition(State::Action, 1u, &anims.SmashForwardAttack, 0u, nullptr);

            else if (mActiveAction->type == ActionType::SmashUp)
                state_transition(State::Action, 1u, &anims.SmashUpAttack, 0u, nullptr);

            else SQASSERT(false, "only smash attacks can be charged");

            mActiveAction->do_start();
        }
    }

    CASE ( Action ) //========================================//
    {
        SQASSERT(mActiveAction != nullptr, "no active action for state 'Action'");

        if (mActiveAction->get_status() == ActionStatus::AllowInterrupt)
        {
            SWITCH ( mActiveAction->type ) {

            CASE ( NeutralFirst )
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);

            CASE ( DashAttack, TiltForward, TiltUp )
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);

            CASE ( TiltDown )
            state_transition(State::Crouch, 0u, nullptr, 0u, &anims.CrouchLoop);

            CASE ( EvadeBack, EvadeForward, Dodge )
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);

            CASE ( ProneAttack, ProneBack, ProneForward, ProneStand )
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);

            CASE ( LedgeClimb )
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);

            CASE ( SmashDown, SmashForward, SmashUp )
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);

            CASE ( LandLight, LandHeavy )
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);

            CASE ( LandTumble )
            state_transition(State::Prone, 0u, nullptr, 0u, &anims.ProneLoop);

            CASE ( LandAirBack, LandAirDown, LandAirForward, LandAirNeutral, LandAirUp )
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);

            CASE ( SpecialDown, SpecialForward, SpecialNeutral, SpecialUp )
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop); // todo

            CASE_DEFAULT { SQASSERT(false, "invalid action for state 'Action'"); }

            } SWITCH_END;
        }

        else SQASSERT(mActiveAction->get_status() == ActionStatus::Running, "");
    }

    CASE ( AirAction ) //=====================================//
    {
        SQASSERT(mActiveAction != nullptr, "no active action for state 'AirAction'");

        if (mActiveAction->get_status() == ActionStatus::AllowInterrupt)
        {
            SWITCH ( mActiveAction->type ) {

            CASE ( AirBack, AirDown, AirForward, AirNeutral, AirUp )
            state_transition(State::JumpFall, 0u, nullptr, 0u, &anims.FallingLoop);

            CASE ( AirDodge )
            state_transition(State::JumpFall, 0u, nullptr, 0u, &anims.FallingLoop); // todo

            CASE ( SpecialDown, SpecialForward, SpecialNeutral, SpecialUp )
            state_transition(State::JumpFall, 0u, nullptr, 0u, &anims.FallingLoop); // todo

            CASE_DEFAULT { SQASSERT(false, "invalid action for state 'AirAction'"); }

            } SWITCH_END;
        }

        else SQASSERT(mActiveAction->get_status() == ActionStatus::Running, "");
    }

    CASE ( Freeze ) //========================================//
    {
        if (mStateProgress == mFreezeTime)
        {
            mStateProgress = mFrozenProgress;
            status.state = mFrozenState;
        }
    }

    //== Nothing to do here ==================================//

    CASE ( EditorPreview ) {}

    } SWITCH_END;
}

//============================================================================//

void Fighter::update_states(const InputFrame& input)
{
    // Everything else happens in here. This function handles:
    // - friction, gravity, knockback decay
    // - interacting with the stage and performing movement
    // - transitions based on falling and landing
    // - passive state updates that don't involve transitions

    Stage& stage = world.get_stage();

    const Animations& anims = mAnimations;

    mStateProgress += 1u;

    //-- freeze state is frozen, so do nothing ---------------//

    if (status.state == State::Freeze)
    {
        return; // EARLY RETURN
    }

    //-- most updates don't apply when ledge hanging ---------//

    if (status.state == State::LedgeHang)
    {
        SQASSERT(status.ledge != nullptr, "nope");

        // this will only be the case if we just grabbed the ledge
        if (status.velocity != Vec2F())
        {
            status.position = (status.position + status.ledge->position) / 2.f;
            status.velocity = Vec2F();
        }
        else status.position = status.ledge->position;

        return; // EARLY RETURN
    }

    mTimeSinceLedge += 1u;

    //-- misc non-transition state updates -------------------//

    if (status.state == State::PreJump)
    {
        if (input.hold_jump == false)
            mJumpHeld = false;
    }

    //-- apply friction --------------------------------------//

    SWITCH ( status.state ) {

    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Prone, Stun, Charge, Action, Shield )
    {
        if (status.velocity.x < -0.f) status.velocity.x = maths::min(status.velocity.x + stats.traction, -0.f);
        if (status.velocity.x > +0.f) status.velocity.x = maths::max(status.velocity.x - stats.traction, +0.f);
    }

    CASE ( JumpFall, TumbleFall, AirStun, TumbleStun, Helpless, AirAction )
    {
        if (input.int_axis.x == 0 && mLaunchSpeed == 0.f)
        {
            if (status.velocity.x < -0.f) status.velocity.x = maths::min(status.velocity.x + stats.air_friction, -0.f);
            if (status.velocity.x > +0.f) status.velocity.x = maths::max(status.velocity.x - stats.air_friction, +0.f);
        }
    }

    CASE ( PreJump, Freeze, LedgeHang, EditorPreview ) {}

    } SWITCH_END;

    //-- apply knockback decay -------------------------------//

    if (mLaunchSpeed != 0.f)
    {
        if (status.velocity != Vec2F())
        {
            const Vec2F direction = maths::normalize(status.velocity);

            if (mLaunchSpeed >= KNOCKBACK_DECAY)
            {
                status.velocity -= direction * KNOCKBACK_DECAY;
                mLaunchSpeed -= KNOCKBACK_DECAY;
            }
            else
            {
                status.velocity -= direction * mLaunchSpeed;
                mLaunchSpeed = 0.f;
            }
        }
        else mLaunchSpeed = 0.f;
    };

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
        if (status.state == State::JumpFall || status.state == State::TumbleFall || status.state == State::Helpless)
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
    const Vec2F targetPosition = status.position + translation;

    //-- ask the stage where we can move ---------------------//

    const bool edgeStop = ( status.state == State::Neutral || status.state == State::Shield || status.state == State::PreJump ||
                            status.state == State::Action || status.state == State::Charge ) ||
                          ( ( status.state == State::Walking || status.state == State::Dashing || status.state == State::Brake ) &&
                            !( input.int_axis.x == -2 && translation.x < -0.0f ) &&
                            !( input.int_axis.x == +2 && translation.x > +0.0f ) );

    MoveAttempt moveAttempt = stage.attempt_move(mLocalDiamond, status.position, targetPosition, edgeStop);

    status.position = moveAttempt.result;

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

    //-- handle landing and starting to fall -----------------//

    SWITCH (status.state) {

    // normal air state, can do light or heavy landing
    CASE ( JumpFall )
    {
        if (moveAttempt.collideFloor == true)
        {
            // todo: this is wrong, need a 'timeSinceFallSpeed' variable
            if (status.velocity.y > -stats.fall_speed || mStateProgress < stats.land_heavy_min_time)
                state_transition_action(ActionType::LandLight);
            else
                state_transition_action(ActionType::LandHeavy);
        }
    }

    // causes the fighter to land awkwardly and become prone
    CASE ( TumbleFall, TumbleStun, Helpless )
    {
        if (moveAttempt.collideFloor == true)
        {
            state_transition_action(ActionType::LandTumble);
            mLaunchSpeed = 0.f;
        }
    }

    // this is an estimated guess based on what brawl seems to do
    CASE ( AirStun )
    {
        if (moveAttempt.collideFloor == true)
        {
            state_transition_action(ActionType::LandHeavy);
            mLaunchSpeed = 0.f;
        }
    }

    // air actions all have different amounts of landing lag
    CASE ( AirAction )
    {
        if (moveAttempt.collideFloor == true)
        {
            if (status.autocancel == false)
            {
                SWITCH (mActiveAction->type) {
                CASE (AirBack)    state_transition_action(ActionType::LandAirBack);
                CASE (AirDown)    state_transition_action(ActionType::LandAirDown);
                CASE (AirForward) state_transition_action(ActionType::LandAirForward);
                CASE (AirNeutral) state_transition_action(ActionType::LandAirNeutral);
                CASE (AirUp)      state_transition_action(ActionType::LandAirUp);
                CASE (AirDodge)   state_transition_action(ActionType::LandHeavy); // todo
                CASE_DEFAULT { SQASSERT(false, "invalid air action"); }
                } SWITCH_END;
            }
            else state_transition_action(ActionType::LandHeavy); // todo: light landings, maybe?
        }
    }

    // misc ground states that you can fall from
    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Charge, Action )
    {
        if (moveAttempt.collideFloor == false)
        {
            // todo: actions probably need special handling here
            cancel_active_action();

            state_transition(State::JumpFall, 4u, &anims.FallingLoop, 0u, nullptr);
            mExtraJumps = stats.extra_jumps;
        }
        else status.velocity.y = 0.f;
    }

    // if we get pushed off of an edge, then we enter tumble
    CASE ( Shield, Prone, Stun )
    {
        if (moveAttempt.collideFloor == false)
        {
            // todo: this should have it's own animation
            state_transition(State::TumbleFall, 4u, &anims.TumbleLoop, 0u, nullptr);
            mExtraJumps = stats.extra_jumps;
        }
        else status.velocity.y = 0.f;
    }

    // special states that you can't fall or land from
    CASE ( PreJump, Freeze, LedgeHang, EditorPreview ) {}

    } SWITCH_END;

    //-- check if we've hit a ceiling or wall ----------------//

    if (moveAttempt.collideCeiling == true)
        status.velocity.y = 0.f;

    if (moveAttempt.collideWall == true)
        status.velocity.x = 0.f;
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

    // rotated because we change facing as soon as a turn anim starts
    const QuatF restRotate = QuatF(0.f, 0.5f, 0.f) * QuatF(-0.25f, 0.f, 0.f);
    const QuatF invRestRotate = QuatF(+0.25f, 0.f, 0.f);

    // called when mAnimation finishes for non-looping animations
    const auto finish_animation = [&]()
    {
        mAnimation = mNextAnimation;
        mNextAnimation = nullptr;

        mAnimTimeDiscrete = 0u;
        mAnimTimeContinuous = 0.f;

        mFadeProgress = 0u;

        mFadeStartRotation = current.rotation;
        mFadeStartPose = current.pose;
    };

    //--------------------------------------------------------//

    const auto set_current_pose_discrete = [&](const Animation& anim, uint time)
    {
        current.pose = mArmature.compute_pose_discrete(anim.anim, time);

        if (world.options.log_animation == true)
            sq::log_debug("pose: {} - {}", anim.key, time);

        mPreviousAnimation = &anim;
        mPreviousAnimTimeDiscrete = time;
    };

    const auto set_current_pose_continuous = [&](const Animation& anim, float time)
    {
        current.pose = mArmature.compute_pose_continuous(anim.anim, time);

        if (world.options.log_animation == true)
            sq::log_debug("pose: {} - {}", anim.key, time);

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

            current.pose[0].offset = Vec3F();
        }

        CASE (DashCycle) //=======================================//
        {
            SQASSERT(mAnimation->anim.looping() == true, "");
            SQASSERT(mNextAnimation == nullptr, "");

            const float distance = std::abs(status.velocity.x);
            const float length = stats.anim_dash_stride / float(mAnimation->anim.totalTime);

            mAnimTimeContinuous += distance / length;

            set_current_pose_continuous(*mAnimation, mAnimTimeContinuous);

            current.pose[0].offset = Vec3F();
        }

        CASE (ApplyMotion) //=====================================//
        {
            SQASSERT(mAnimation->anim.looping() == false, "");

            if (mAnimTimeDiscrete == 0u)
                mPrevRootMotionOffset = Vec3F();

            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            const Vec3F offsetLocal = current.pose[0].offset - mPrevRootMotionOffset;
            mPrevRootMotionOffset = current.pose[0].offset;

            mTranslate += { offsetLocal.z * float(status.facing), offsetLocal.x };
            current.translation += Vec3F(mTranslate, 0.f);
            current.pose[0].offset = Vec3F();

            if (++mAnimTimeDiscrete == mAnimation->anim.totalTime)
                finish_animation();
        }

        CASE (ApplyTurn) //=======================================//
        {
            SQASSERT(mAnimation->anim.looping() == false, "");

            if (mAnimTimeDiscrete == 0u)
                mPrevRootMotionOffset = Vec3F();

            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            current.rotation = restRotate * current.pose[2].rotation * invRestRotate * current.rotation;
            current.pose[2].rotation = QuatF();

            if (++mAnimTimeDiscrete == mAnimation->anim.totalTime)
                finish_animation();
        }

        CASE (MotionTurn) //======================================//
        {
            SQASSERT(mAnimation->anim.looping() == false, "");

            if (mAnimTimeDiscrete == 0u)
                mPrevRootMotionOffset = Vec3F();

            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            const Vec3F offsetLocal = current.pose[0].offset - mPrevRootMotionOffset;
            mPrevRootMotionOffset = current.pose[0].offset;

            mTranslate += { offsetLocal.z * float(status.facing) * -1.f, offsetLocal.x };
            current.translation += Vec3F(mTranslate, 0.f);
            current.pose[0].offset = Vec3F();

            current.rotation = restRotate * current.pose[2].rotation * invRestRotate * current.rotation;
            current.pose[2].rotation = QuatF();

            if (++mAnimTimeDiscrete == mAnimation->anim.totalTime)
                finish_animation();
        }

        } SWITCH_END;
    }

    //-- blend from fade pose for smooth transitions ---------//

    if (mFadeProgress != mFadeFrames)
    {
        const float blend = float(++mFadeProgress) / float(mFadeFrames + 1u);

        current.rotation = maths::slerp(mFadeStartRotation, current.rotation, blend);
        current.pose = mArmature.blend_poses(mFadeStartPose, current.pose, blend);

        if (world.options.log_animation == true)
            sq::log_debug("blend - {} / {} - {}", mFadeProgress, mFadeFrames, blend);
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

    const auto input = world.options.editor_mode ? InputFrame() : mController->get_input();

    update_commands(input);
    //update_action(input);
    update_transitions(input);
    update_states(input);
    update_action(input);

    if (status.state != State::Freeze || mStateProgress <= 1u)
    {
        current.translation = Vec3F(status.position, 0.f);

        if (status.state == State::EditorPreview) current.rotation = QuatF(0.f, 0.5f, 0.f);
        else current.rotation = QuatF(0.f, 0.25f * float(status.facing), 0.f);

        update_animation();
    }

    mArmature.compute_ubo_data(current.pose, mBoneMatrices.data(), uint(mBoneMatrices.size()));

    mModelMatrix = maths::transform(current.translation, current.rotation);
}
