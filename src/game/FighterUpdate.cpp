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
    SQASSERT(fadeNow == 0u || animNow != nullptr, "can't fade now without animation");
    SQASSERT(fadeAfter == 0u || animAfter != nullptr, "can't fade after without animation");
    SQASSERT(animAfter == nullptr || animAfter->is_looping(), "anim after must loop");

    status.state = state;
    mStateProgress = 0u;

    // current animation will be kept if animNow is null
    if (animNow != nullptr)
    {
        // don't restart already playing loop animations
        if (mAnimation != animNow || !mAnimation->is_looping())
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
    // This function gets called whenever we want to do a state transition as well as
    // start an action. It is intentionally not possible to start an action without
    // performing the transition defined in this function.

    const Animations& anims = mAnimations;

    cancel_active_action();

    SWITCH ( action ) {

    CASE (NeutralFirst)  state_transition(State::Action, 1u, &anims.NeutralFirst, 0u, nullptr);
    CASE (NeutralSecond) state_transition(State::Action, 1u, &anims.NeutralSecond, 0u, nullptr);
    CASE (NeutralThird)  state_transition(State::Action, 1u, &anims.NeutralThird, 0u, nullptr);

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

    CASE (ChargeDown)    state_transition(State::Action, 1u, &anims.SmashDownStart, 0u, &anims.SmashDownCharge);
    CASE (ChargeForward) state_transition(State::Action, 1u, &anims.SmashForwardStart, 0u, &anims.SmashForwardCharge);
    CASE (ChargeUp)      state_transition(State::Action, 1u, &anims.SmashUpStart, 0u, &anims.SmashUpCharge);

    CASE (SmashDown)    state_transition(State::Action, 1u, &anims.SmashDownAttack, 0u, nullptr);
    CASE (SmashForward) state_transition(State::Action, 1u, &anims.SmashForwardAttack, 0u, nullptr);
    CASE (SmashUp)      state_transition(State::Action, 1u, &anims.SmashUpAttack, 0u, nullptr);

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

    CASE (ShieldOn)  state_transition(State::Shield, 2u, &anims.ShieldOn, 0u, &anims.ShieldLoop);
    CASE (ShieldOff) state_transition(State::Action, 2u, &anims.ShieldOff, 0u, nullptr);

    CASE (HopBack)       state_transition(State::JumpFall, 1u, &anims.JumpBack, 0u, &anims.FallingLoop);
    CASE (HopForward)    state_transition(State::JumpFall, 1u, &anims.JumpForward, 0u, &anims.FallingLoop);
    CASE (JumpBack)      state_transition(State::JumpFall, 1u, &anims.JumpBack, 0u, &anims.FallingLoop);
    CASE (JumpForward)   state_transition(State::JumpFall, 1u, &anims.JumpForward, 0u, &anims.FallingLoop);
    CASE (AirHopBack)    state_transition(State::JumpFall, 2u, &anims.AirHopBack, 1u, &anims.FallingLoop);
    CASE (AirHopForward) state_transition(State::JumpFall, 2u, &anims.AirHopForward, 1u, &anims.FallingLoop);

    CASE (DashStart) state_transition(State::DashStart, 0u, &anims.DashStart, 0u, nullptr);
    CASE (DashBrake) state_transition(State::Brake, 4u, &anims.Brake, 0u, nullptr);

    CASE (None) SQASSERT(false, "can't switch to None");

    } SWITCH_END;

    if (ActionTraits::get(action).exclusive == true)
        SQASSERT(status.state == State::Action || status.state == State::AirAction, "");

    if (ActionTraits::get(action).aerial == true)
        SQASSERT(status.state == State::JumpFall || status.state == State::AirAction, "");

    mActiveAction = get_action(action);
    mActiveAction->do_start();

    mActiveAction->set_flag(ActionFlag::AttackHeld, true);
    mActiveAction->set_flag(ActionFlag::HitCollide, false);
    mActiveAction->set_flag(ActionFlag::AllowNext, false);
    mActiveAction->set_flag(ActionFlag::AutoJab, false);
}

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
        cmds.push_back(Command::Dodge);

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

void Fighter::update_action(const InputFrame& input)
{
    // actions get updated last so that they tick on the frame they activated,
    // with the exception of actions that get activated here

    if (status.state == State::Freeze) return;
    if (mActiveAction == nullptr) return;

    //--------------------------------------------------------//

    if (input.hold_attack == false)
        mActiveAction->set_flag(ActionFlag::AttackHeld, false);

    mActiveAction->do_tick();

    //--------------------------------------------------------//

    const auto handle_neutral_combo = [&](ActionType nextAction)
    {
        if (mActiveAction->check_flag(ActionFlag::AllowNext))
        {
            if (consume_command(FIGHTER_COMMANDS_ANY_ATTACK))
                state_transition_action(nextAction);

            else if (mActiveAction->check_flag(ActionFlag::AttackHeld))
            {
                if (mActiveAction->check_flag(ActionFlag::HitCollide))
                    state_transition_action(nextAction);

                else if (mActiveAction->check_flag(ActionFlag::AutoJab))
                    state_transition_action(ActionType::NeutralFirst);
            }
        }
    };

    const auto handle_charge_smash = [&](ActionType smashAction)
    {
        if (mActiveAction->check_flag(ActionFlag::AllowNext))
        {
            if (!mActiveAction->check_flag(ActionFlag::AttackHeld))
                state_transition_action(smashAction);

            else if (mActiveAction->get_status() == ActionStatus::Finished)
                state_transition_action(smashAction);
        }
    };

    //--------------------------------------------------------//

    if      (mActiveAction->type == ActionType::NeutralFirst)  handle_neutral_combo(ActionType::NeutralSecond);
    else if (mActiveAction->type == ActionType::NeutralSecond) handle_neutral_combo(ActionType::NeutralThird);

    else if (mActiveAction->type == ActionType::ChargeDown)    handle_charge_smash(ActionType::SmashDown);
    else if (mActiveAction->type == ActionType::ChargeForward) handle_charge_smash(ActionType::SmashForward);
    else if (mActiveAction->type == ActionType::ChargeUp)      handle_charge_smash(ActionType::SmashUp);

    //--------------------------------------------------------//

    if (mActiveAction->get_status() == ActionStatus::Finished)
        mActiveAction = nullptr;
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

    if (mForceSwitchAction != ActionType::None)
    {
       state_transition_action(mForceSwitchAction);
       mForceSwitchAction = ActionType::None;

       return; // early return
    }

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

    SWITCH ( status.state ) {

    CASE ( Neutral ) //=======================================//
    {
        if (consume_command(Command::Jump))
            state_transition(State::PreJump, 1u, &anims.PreJump, 0u, nullptr),
                mJumpHeld = true,
                mExtraJumps = attributes.extra_jumps;

        else if (input.hold_shield == true)
            state_transition_action(ActionType::ShieldOn);

        else if (consume_command(Command::SmashDown))
            state_transition_action(ActionType::ChargeDown);

        else if (consume_command(Command::SmashUp))
            state_transition_action(ActionType::ChargeUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            state_transition_action(ActionType::ChargeForward);

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
            state_transition(State::Neutral, 0u, &anims.Turn, 0u, &anims.NeutralLoop),
                status.facing = -status.facing;

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
        if (consume_command(Command::Jump))
            state_transition(State::PreJump, 1u, &anims.PreJump, 0u, nullptr),
                mJumpHeld = true,
                mExtraJumps = attributes.extra_jumps;

        else if (input.hold_shield == true)
            state_transition_action(ActionType::ShieldOn);

        else if (consume_command(Command::SmashDown))
            state_transition_action(ActionType::ChargeDown);

        else if (consume_command(Command::SmashUp))
            state_transition_action(ActionType::ChargeUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            state_transition_action(ActionType::ChargeForward);

        else if (consume_command(Command::AttackDown))
            state_transition_action(ActionType::TiltDown);

        else if (consume_command(Command::AttackUp))
            state_transition_action(ActionType::TiltUp);

        else if (consume_command_facing(Command::AttackLeft, Command::AttackRight))
            state_transition_action(ActionType::TiltForward);

        else if (consume_command_facing(Command::MashLeft, Command::MashRight))
            state_transition_action(ActionType::DashStart);

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

    CASE ( DashStart ) //=====================================//
    {
        if (consume_command(Command::Jump))
            state_transition(State::PreJump, 1u, &anims.PreJump, 0u, nullptr),
                mJumpHeld = true,
                mExtraJumps = attributes.extra_jumps;

        else if (input.hold_shield == true)
            state_transition_action(ActionType::ShieldOn);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            state_transition_action(ActionType::ChargeForward),
                status.velocity.x = 0.f;

        else if (consume_command({Command::AttackDown, Command::AttackUp, Command::AttackLeft, Command::AttackRight, Command::AttackNeutral}))
            state_transition_action(ActionType::DashAttack);

        else if (mStateProgress == attributes.dash_start_time)
        {
            if (input.int_axis.x != status.facing * 2)
                state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
            else
                state_transition(State::Dashing, 2u, &anims.DashingLoop, 0u, nullptr);
        }

        else if (input.int_axis.x == status.facing * -2)
            state_transition_action(ActionType::DashStart),
                status.facing = -status.facing;
    }

    CASE ( Dashing ) //=======================================//
    {
        if (consume_command(Command::Jump))
            state_transition(State::PreJump, 1u, &anims.PreJump, 0u, nullptr),
                mJumpHeld = true,
                mExtraJumps = attributes.extra_jumps;

        else if (input.hold_shield == true)
            state_transition_action(ActionType::ShieldOn);

        else if (consume_command_facing(Command::AttackLeft, Command::AttackRight))
            state_transition_action(ActionType::DashAttack);

        else if (input.int_axis.x != status.facing * 2)
            state_transition_action(ActionType::DashBrake);
    }

    CASE ( Brake ) //=========================================//
    {
        if (consume_command(Command::Jump))
            state_transition(State::PreJump, 1u, &anims.PreJump, 0u, nullptr),
                mJumpHeld = true,
                mExtraJumps = attributes.extra_jumps;

        else if (input.hold_shield == true)
            state_transition_action(ActionType::ShieldOn);

        else if (consume_command(Command::SmashDown))
            state_transition_action(ActionType::ChargeDown);

        else if (consume_command(Command::SmashUp))
            state_transition_action(ActionType::ChargeUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            state_transition_action(ActionType::ChargeForward);

        else if (consume_command({Command::AttackDown, Command::AttackUp, Command::AttackLeft, Command::AttackRight, Command::AttackNeutral}))
            state_transition_action(ActionType::DashAttack);

        else if (consume_command_facing(Command::TurnRight, Command::TurnLeft))
            state_transition(State::BrakeTurn, 0u, &anims.TurnDash, 0u, nullptr),
                status.facing = -status.facing;

        else if (mStateProgress == attributes.dash_brake_time)
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
    }

    CASE ( BrakeTurn ) //=====================================//
    {
        if (consume_command(Command::Jump))
            state_transition(State::PreJump, 1u, &anims.PreJump, 0u, nullptr),
                mJumpHeld = true,
                mExtraJumps = attributes.extra_jumps;

        else if (input.hold_shield == true)
            state_transition_action(ActionType::ShieldOn);

        else if (consume_command(Command::SmashDown))
            state_transition_action(ActionType::ChargeDown);

        else if (consume_command(Command::SmashUp))
            state_transition_action(ActionType::ChargeUp);

        else if (consume_command_facing(Command::SmashLeft, Command::SmashRight))
            state_transition_action(ActionType::ChargeForward);

        else if (mStateProgress == attributes.dash_turn_time)
        {
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

        if (consume_command(Command::Jump))
            state_transition(State::PreJump, 1u, &anims.PreJump, 0u, nullptr),
                mJumpHeld = true,
                mExtraJumps = attributes.extra_jumps;

        else if (input.hold_shield == true)
            state_transition_action(ActionType::ShieldOn);

        else if (consume_command(Command::SmashDown))
            state_transition_action(ActionType::ChargeDown);

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
                state_transition_action(mJumpHeld ? ActionType::JumpBack : ActionType::HopBack);
            else
                state_transition_action(mJumpHeld ? ActionType::JumpForward : ActionType::HopForward);

            // this could be a seperate stat, but half air speed seems good enough
            status.velocity.x = attributes.air_speed * input.float_axis.x * 0.5f;

            if (mJumpHeld == true)
                status.velocity.y = std::sqrt(2.f * attributes.jump_height * attributes.gravity) + attributes.gravity * 0.5f;
            else
                status.velocity.y = std::sqrt(2.f * attributes.hop_height * attributes.gravity) + attributes.gravity * 0.5f;
        }
    }

    CASE ( Prone ) //=========================================//
    {
        if (consume_command_facing(Command::MashLeft, Command::MashRight))
            state_transition_action(ActionType::ProneForward);

        else if (consume_command_facing(Command::MashRight, Command::MashLeft))
            state_transition_action(ActionType::ProneBack);

        else if (consume_command({Command::MashUp, Command::Jump}))
            state_transition_action(ActionType::ProneStand);

        else if (consume_command({Command::AttackDown, Command::AttackUp, Command::AttackLeft, Command::AttackRight, Command::AttackNeutral}))
            state_transition_action(ActionType::ProneAttack);
    }

    CASE ( Shield ) //========================================//
    {
        if (consume_command(Command::Jump))
            state_transition(State::PreJump, 1u, &anims.PreJump, 0u, nullptr),
                mJumpHeld = true,
                mExtraJumps = attributes.extra_jumps;

        else if (consume_command_facing(Command::MashLeft, Command::MashRight))
            state_transition_action(ActionType::EvadeForward),
                status.facing = -status.facing;

        else if (consume_command_facing(Command::MashRight, Command::MashLeft))
            state_transition_action(ActionType::EvadeBack);

        else if (consume_command({Command::MashDown, Command::MashUp}))
            state_transition_action(ActionType::Dodge);

        else if (input.hold_shield == false)
            state_transition_action(ActionType::ShieldOff);
    }

    CASE ( JumpFall, TumbleFall, FastFall ) //================//
    {
        if (try_catch_ledge() == true)
            state_transition(State::LedgeHang, 1u, &anims.LedgeCatch, 0u, &anims.LedgeLoop);

        else if (consume_command(Command::Dodge))
            state_transition_action(ActionType::AirDodge);

        else if (mExtraJumps > 0u && consume_command(Command::Jump))
        {
            if (input.norm_axis.x == -status.facing)
                state_transition_action(ActionType::AirHopBack);
            else
                state_transition_action(ActionType::AirHopForward);

            mExtraJumps -= 1u;
            status.velocity.y = std::sqrt(2.f * attributes.gravity * attributes.airhop_height) + attributes.gravity * 0.5f;
        }

        else if (consume_command({Command::SmashDown, Command::AttackDown}))
            state_transition_action(ActionType::AirDown);

        else if (consume_command({Command::SmashUp, Command::AttackUp}))
            state_transition_action(ActionType::AirUp);

        else if (consume_command_facing({Command::SmashLeft, Command::AttackLeft}, {Command::SmashRight, Command::AttackRight}))
            state_transition_action(ActionType::AirForward);

        else if (consume_command_facing({Command::SmashRight, Command::AttackRight}, {Command::SmashLeft, Command::AttackLeft}))
            state_transition_action(ActionType::AirBack);

        else if (consume_command(Command::AttackNeutral))
            state_transition_action(ActionType::AirNeutral);

        else if (status.state == State::JumpFall && status.velocity.y < 0.f && consume_command(Command::MashDown))
            state_transition(State::FastFall, 0u, nullptr, 0u, nullptr);
    }

    CASE ( Stun ) //==========================================//
    {
        if (mStateProgress == mHitStunTime)
            state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
    }

    CASE ( ShieldStun ) //====================================//
    {
        if (mStateProgress == mHitStunTime)
            state_transition(State::Shield, 0u, nullptr, 0u, &anims.ShieldLoop);
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
            mExtraJumps = attributes.extra_jumps;
        }

        else if (mStateProgress >= LEDGE_HANG_MIN_TIME)
        {
            if (consume_command(Command::Jump))
            {
                state_transition(State::JumpFall, 1u, &anims.LedgeJump, 0u, &anims.FallingLoop);
                status.velocity.y = std::sqrt(2.f * attributes.jump_height * attributes.gravity) + attributes.gravity * 0.5f;
                status.ledge->grabber = nullptr;
                status.ledge = nullptr;
                mExtraJumps = attributes.extra_jumps;
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
                mExtraJumps = attributes.extra_jumps;
            }
        }
    }

    CASE ( Action ) //========================================//
    {
        SQASSERT(mActiveAction != nullptr, "no active action for state 'Action'");

        if (mActiveAction->get_status() == ActionStatus::AllowInterrupt || mActiveAction->get_status() == ActionStatus::RuntimeError)
        {
            if (mActiveAction->type == ActionType::TiltDown)
                state_transition(State::Crouch, 0u, nullptr, 0u, &anims.CrouchLoop);

            else if (mActiveAction->type == ActionType::LandTumble)
                state_transition(State::Prone, 0u, nullptr, 0u, &anims.ProneLoop);

            else
                state_transition(State::Neutral, 0u, nullptr, 0u, &anims.NeutralLoop);
        }
        else SQASSERT(mActiveAction->get_status() == ActionStatus::Running, "unexpected state");
    }

    CASE ( AirAction ) //=====================================//
    {
        SQASSERT(mActiveAction != nullptr, "no active action for state 'AirAction'");

        if (mActiveAction->get_status() == ActionStatus::AllowInterrupt || mActiveAction->get_status() == ActionStatus::RuntimeError)
        {
            if (true == true)
                state_transition(State::JumpFall, 0u, nullptr, 0u, &anims.FallingLoop);
        }
        else SQASSERT(mActiveAction->get_status() == ActionStatus::Running, "unexpected state");
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

    if (input.hold_jump == false)
        mJumpHeld = false;

    if (-status.velocity.y < attributes.fall_speed) mTimeSinceFallSpeed = 0u;
    else mTimeSinceFallSpeed += 1u;

    //-- apply friction --------------------------------------//

    SWITCH ( status.state ) {

    CASE ( Neutral, Walking, DashStart, Dashing, Brake, BrakeTurn, Crouch, Prone, Shield, Stun, ShieldStun, Action )
    {
        if (status.velocity.x < -0.f) status.velocity.x = maths::min(status.velocity.x + attributes.traction, -0.f);
        if (status.velocity.x > +0.f) status.velocity.x = maths::max(status.velocity.x - attributes.traction, +0.f);
    }

    CASE ( JumpFall, TumbleFall, FastFall, AirStun, TumbleStun, Helpless, AirAction )
    {
        if (input.int_axis.x == 0 && mLaunchSpeed == 0.f)
        {
            if (status.velocity.x < -0.f) status.velocity.x = maths::min(status.velocity.x + attributes.air_friction, -0.f);
            if (status.velocity.x > +0.f) status.velocity.x = maths::max(status.velocity.x - attributes.air_friction, +0.f);
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
        if ( status.state == State::JumpFall || status.state == State::TumbleFall || status.state == State::FastFall ||
             status.state == State::Helpless || status.state == State::AirAction )
            apply_horizontal_move(attributes.air_mobility, attributes.air_speed);

        else if (status.state == State::Walking)
            apply_horizontal_move(attributes.traction * 2.f, attributes.walk_speed);

        else if (status.state == State::DashStart)
            apply_horizontal_move(attributes.traction * 8.f, attributes.dash_speed);

        else if (status.state == State::Dashing)
            apply_horizontal_move(attributes.traction * 4.f, attributes.dash_speed);
    }

    //-- apply gravity, velocity, and translation ------------//

    if (status.state == State::FastFall) status.velocity.y = -attributes.fastfall_speed;
    else status.velocity.y = maths::max(status.velocity.y - attributes.gravity, -attributes.fall_speed);

    const Vec2F translation = status.velocity + mTranslate;
    const Vec2F targetPosition = status.position + translation;

    //-- ask the stage where we can move ---------------------//

    const bool edgeStop = ( status.state == State::Neutral || status.state == State::Shield ||
                            status.state == State::PreJump || status.state == State::Action ) ||
                          ( ( status.state == State::Walking || status.state == State::DashStart ||
                              status.state == State::Dashing || status.state == State::Brake ||
                              status.state == State::BrakeTurn ) &&
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
            if (mTimeSinceFallSpeed < attributes.land_heavy_fall_time)
                state_transition_action(ActionType::LandLight);
            else
                state_transition_action(ActionType::LandHeavy);
        }
    }

    // fastfall always does a heavy landing
    CASE ( FastFall )
    {
        if (moveAttempt.collideFloor == true)
            state_transition_action(ActionType::LandHeavy);
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
    CASE ( Neutral, Walking, DashStart, Dashing, Brake, BrakeTurn, Crouch, Action )
    {
        if (moveAttempt.collideFloor == false)
        {
            // todo: actions probably need special handling here
            cancel_active_action();

            state_transition(State::JumpFall, 4u, &anims.FallingLoop, 0u, nullptr);
            mExtraJumps = attributes.extra_jumps;
        }
        else status.velocity.y = 0.f;
    }

    // if we get pushed off of an edge, then we enter tumble
    CASE ( Shield, Prone, Stun, ShieldStun )
    {
        if (moveAttempt.collideFloor == false)
        {
            // todo: this should have it's own animation
            state_transition(State::TumbleFall, 4u, &anims.TumbleLoop, 0u, nullptr);
            mExtraJumps = attributes.extra_jumps;
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

    //-- decay or regenerate our shield ----------------------//

    if (status.state == State::Shield)
    {
        status.shield -= SHIELD_DECAY;
        if (status.shield <= 0.f)
            apply_shield_break(); // todo
    }

    else if (status.state != State::ShieldStun)
        status.shield = maths::min(status.shield + SHIELD_REGEN, SHIELD_MAX_HP);
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
            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
                finish_animation();
        }

        CASE (Looping) //=========================================//
        {
            SQASSERT(mNextAnimation == nullptr, "");

            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
                mAnimTimeDiscrete = 0u;
        }

        CASE (WalkCycle) //=======================================//
        {
            SQASSERT(mNextAnimation == nullptr, "");

            const float distance = std::abs(status.velocity.x);
            const float animSpeed = attributes.anim_walk_stride / float(mAnimation->anim.frameCount);

            mAnimTimeContinuous += distance / animSpeed;

            set_current_pose_continuous(*mAnimation, mAnimTimeContinuous);

            current.pose[0].offset = Vec3F();
        }

        CASE (DashCycle) //=======================================//
        {
            SQASSERT(mNextAnimation == nullptr, "");

            const float distance = std::abs(status.velocity.x);
            const float length = attributes.anim_dash_stride / float(mAnimation->anim.frameCount);

            mAnimTimeContinuous += distance / length;

            set_current_pose_continuous(*mAnimation, mAnimTimeContinuous);

            current.pose[0].offset = Vec3F();
        }

        CASE (ApplyMotion) //=====================================//
        {
            if (mAnimTimeDiscrete == 0u)
                mPrevRootMotionOffset = Vec3F();

            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            const Vec3F offsetLocal = current.pose[0].offset - mPrevRootMotionOffset;
            mPrevRootMotionOffset = current.pose[0].offset;

            mTranslate += { offsetLocal.z * float(status.facing), offsetLocal.x };
            current.translation += Vec3F(mTranslate, 0.f);
            current.pose[0].offset = Vec3F();

            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
                finish_animation();
        }

        CASE (ApplyTurn) //=======================================//
        {
            if (mAnimTimeDiscrete == 0u)
                mPrevRootMotionOffset = Vec3F();

            set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

            current.rotation = restRotate * current.pose[2].rotation * invRestRotate * current.rotation;
            current.pose[2].rotation = QuatF();

            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
                finish_animation();
        }

        CASE (MotionTurn) //======================================//
        {
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

            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
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

void Fighter::tick()
{
    previous = current;

    const auto input = world.options.editor_mode ? InputFrame() : mController->get_input();

    update_commands(input);
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

    // in smash proper, this is done with a flag that gets set by various actions
    // this is much more limited, but works well enough for now

    const bool stun = status.state == State::Stun || status.state == State::AirStun || status.state == State::TumbleStun;
    const bool freezeStun = mFrozenState == State::Stun || mFrozenState == State::AirStun || mFrozenState == State::TumbleStun;
    const bool tumbleState = status.state == State::TumbleFall || status.state == State::Prone;
    const bool tumbleAction = status.state == State::Action && mActiveAction->type == ActionType::LandTumble;

    status.flinch = stun || (status.state == State::Freeze && freezeStun) || tumbleState || tumbleAction;
}
