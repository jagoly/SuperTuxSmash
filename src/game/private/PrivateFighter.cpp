#include "game/private/PrivateFighter.hpp"

#include "game/Stage.hpp"

#include <sqee/macros.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

namespace maths = sq::maths;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

static constexpr uint STS_LIGHT_LANDING_LAG = 2u;

static constexpr uint STS_HEAVY_LANDING_LAG = 8u;

static constexpr uint STS_JUMP_DELAY = 5u;

static constexpr uint STS_NO_LEDGE_CATCH_TIME = 48u;

static constexpr uint STS_MIN_LEDGE_HANG_TIME = 16u;

//============================================================================//

void PrivateFighter::initialise_armature(const String& path)
{
    armature.load_bones(path + "/Bones.txt", true);
    armature.load_rest_pose(path + "/RestPose.txt");

    current.pose = previous.pose = armature.get_rest_pose();

    fighter.mBoneMatrices.resize(armature.get_bone_count());

    //--------------------------------------------------------//

    const auto load_anim = [&](const char* name, AnimMode mode) -> Animation
    {
        const bool looping = mode == AnimMode::Looping || mode == AnimMode::WalkCycle || mode == AnimMode::DashCycle;

        String filePath = sq::build_path(path, "anims", name) + ".sqa";
        if (sq::check_file_exists(filePath) == false)
        {
            filePath = sq::build_path(path, "anims", name) + ".txt";
            if (sq::check_file_exists(filePath) == false)
            {
                sq::log_warning("missing animation '%s'", filePath);
                return { armature.make_null_animation(1u, looping), mode, name };
            }
        }
        Animation result = { armature.make_animation(filePath), mode, name };
        if (!result.anim.looping() && looping) sq::log_warning("animation '%s' should loop", filePath);
        if (result.anim.looping() && !looping) sq::log_warning("animation '%s' should not loop", filePath);
        return result;
    };

    animations.DashingLoop = load_anim("DashingLoop", AnimMode::DashCycle);
    animations.FallingLoop = load_anim("FallingLoop", AnimMode::Looping);
    animations.NeutralLoop = load_anim("NeutralLoop", AnimMode::Looping);
    animations.VertigoLoop = load_anim("VertigoLoop", AnimMode::Looping);
    animations.WalkingLoop = load_anim("WalkingLoop", AnimMode::WalkCycle);

    animations.ShieldOn = load_anim("ShieldOn", AnimMode::Standard);
    animations.ShieldOff = load_anim("ShieldOff", AnimMode::Standard);
    animations.ShieldLoop = load_anim("ShieldLoop", AnimMode::Looping);

    animations.CrouchOn = load_anim("CrouchOn", AnimMode::Standard);
    animations.CrouchOff = load_anim("CrouchOff", AnimMode::Standard);
    animations.CrouchLoop = load_anim("CrouchLoop", AnimMode::Looping);

    animations.DashStart = load_anim("DashStart", AnimMode::Standard);
    animations.VertigoStart = load_anim("VertigoStart", AnimMode::Standard);

    animations.Brake = load_anim("Brake", AnimMode::Standard);
    animations.LandLight = load_anim("LandLight", AnimMode::Standard);
    animations.LandHeavy = load_anim("LandHeavy", AnimMode::Standard);
    animations.PreJump = load_anim("PreJump", AnimMode::Standard);
    animations.Turn = load_anim("Turn", AnimMode::Standard);
    animations.TurnBrake = load_anim("TurnBrake", AnimMode::Standard);
    animations.TurnDash = load_anim("TurnDash", AnimMode::Standard);

    animations.JumpBack = load_anim("JumpBack", AnimMode::Standard);
    animations.JumpForward = load_anim("JumpForward", AnimMode::Standard);
    animations.AirHopBack = load_anim("AirHopBack", AnimMode::Standard);
    animations.AirHopForward = load_anim("AirHopForward", AnimMode::Standard);

    animations.LedgeCatch = load_anim("LedgeCatch", AnimMode::Standard);
    animations.LedgeLoop = load_anim("LedgeLoop", AnimMode::Looping);
    animations.LedgeClimb = load_anim("LedgeClimb", AnimMode::ApplyMotion);
    animations.LedgeJump = load_anim("LedgeJump", AnimMode::Standard);

    animations.EvadeBack = load_anim("EvadeBack", AnimMode::ApplyMotion);
    animations.EvadeForward = load_anim("EvadeForward", AnimMode::ApplyMotion);
    animations.Dodge = load_anim("Dodge", AnimMode::Standard);
    animations.AirDodge = load_anim("AirDodge", AnimMode::Standard);

    animations.NeutralFirst = load_anim("NeutralFirst", AnimMode::Standard);

    animations.TiltDown = load_anim("TiltDown", AnimMode::Standard);
    animations.TiltForward = load_anim("TiltForward", AnimMode::ApplyMotion);
    animations.TiltUp = load_anim("TiltUp", AnimMode::Standard);

    animations.AirBack = load_anim("AirBack", AnimMode::Standard);
    animations.AirDown = load_anim("AirDown", AnimMode::Standard);
    animations.AirForward = load_anim("AirForward", AnimMode::Standard);
    animations.AirNeutral = load_anim("AirNeutral", AnimMode::Standard);
    animations.AirUp = load_anim("AirUp", AnimMode::Standard);

    animations.DashAttack = load_anim("DashAttack", AnimMode::ApplyMotion);

    animations.SmashDownStart = load_anim("SmashDownStart", AnimMode::Standard);
    animations.SmashForwardStart = load_anim("SmashForwardStart", AnimMode::ApplyMotion);
    animations.SmashUpStart = load_anim("SmashUpStart", AnimMode::Standard);

    animations.SmashDownCharge = load_anim("SmashDownCharge", AnimMode::Looping);
    animations.SmashForwardCharge = load_anim("SmashForwardCharge", AnimMode::Looping);
    animations.SmashUpCharge = load_anim("SmashUpCharge", AnimMode::Looping);

    animations.SmashDownAttack = load_anim("SmashDownAttack", AnimMode::Standard);
    animations.SmashForwardAttack = load_anim("SmashForwardAttack", AnimMode::ApplyMotion);
    animations.SmashUpAttack = load_anim("SmashUpAttack", AnimMode::Standard);

    // some anims need to be longer than a certain length for logic to work correctly
    const auto ensure_anim_time = [](Animation& anim, uint time, const char* animName, const char* timeName)
    {
        if (anim.anim.totalTime > time) return; // anim is longer than time

        if (anim.anim.totalTime != 1u) // fallback animation, don't print another warning
            sq::log_warning("anim '%s' shorter than '%s'", animName, timeName);

        anim.anim.totalTime = anim.anim.times.front() = time + 1u;
    };

    ensure_anim_time(animations.DashStart, fighter.stats.dash_start_time, "DashStart", "dash_start_time");
    ensure_anim_time(animations.Brake, fighter.stats.dash_brake_time, "Brake", "dash_brake_time");
    ensure_anim_time(animations.TurnDash, fighter.stats.dash_turn_time, "TurnDash", "dash_turn_time");
    ensure_anim_time(animations.LedgeClimb, fighter.stats.ledge_climb_time, "LedgeClimb", "ledge_climb_time");
}

//============================================================================//

void PrivateFighter::initialise_hurt_blobs(const String& path)
{
    const JsonValue root = sq::parse_json_from_file(path + "/HurtBlobs.json");
    for (const auto& item : root.items())
    {
        HurtBlob& blob = fighter.hurtBlobs[item.key()];

        blob.fighter = &fighter;

        try { blob.from_json(item.value()); }
        catch (const std::exception& e) {
            sq::log_warning("problem loading hurt blob '%s': %s", item.key(), e.what());
        }

        get_world().enable_hurt_blob(&blob);
    }
}

//============================================================================//

void PrivateFighter::initialise_stats(const String& path)
{
    Fighter::Stats& stats = fighter.stats;

    const auto json = sq::parse_json_from_file(path + "/Stats.json");

    stats.walk_speed    = json.at("walk_speed");
    stats.dash_speed    = json.at("dash_speed");
    stats.air_speed     = json.at("air_speed");
    stats.traction      = json.at("traction");
    stats.air_mobility  = json.at("air_mobility");
    stats.air_friction  = json.at("air_friction");
    stats.hop_height    = json.at("hop_height");
    stats.jump_height   = json.at("jump_height");
    stats.airhop_height = json.at("airhop_height");
    stats.gravity       = json.at("gravity");
    stats.fall_speed    = json.at("fall_speed");
    stats.weight        = json.at("weight");

    stats.extra_jumps = json.at("extra_jumps");

    stats.land_heavy_min_time = json.at("land_heavy_min_time");

    stats.dash_start_time  = json.at("dash_start_time");
    stats.dash_brake_time  = json.at("dash_brake_time");
    stats.dash_turn_time   = json.at("dash_turn_time");
    stats.ledge_climb_time = json.at("ledge_climb_time");

    stats.anim_walk_stride = json.at("anim_walk_stride");
    stats.anim_dash_stride = json.at("anim_dash_stride");
}

//============================================================================//

void PrivateFighter::initialise_actions(const String& path)
{
    for (int8_t i = 0; i < sq::enum_count_v<ActionType>; ++i)
    {
        const auto actionType = ActionType(i);
        const auto actionPath = sq::build_string(path, "/actions/", sq::to_c_string(actionType));

        fighter.actions[i] = std::make_unique<Action>(get_world(), fighter, actionType, actionPath);

        fighter.actions[i]->load_from_json();
        fighter.actions[i]->load_lua_from_file();
    }
}

//============================================================================//

void PrivateFighter::state_transition(State newState, uint fadeNow, const Animation* animNow, uint fadeAfter, const Animation* animAfter)
{
    SQASSERT(fadeNow == 0u || animNow != nullptr, "");
    SQASSERT(fadeAfter == 0u || animAfter != nullptr, "");
    SQASSERT(newState == State::EditorPreview || animNow != nullptr || animAfter != nullptr, "");

    fighter.current.state = newState;
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

void PrivateFighter::switch_action(ActionType type)
{
    Action* const newAction = fighter.get_action(type);

    SQASSERT(newAction != fighter.current.action, "switch to same action");

    //--------------------------------------------------------//

    SWITCH ( type ) {

    //--------------------------------------------------------//

    CASE (NeutralFirst) state_transition(State::Action, 1u, &animations.NeutralFirst, 0u, nullptr);

    CASE (TiltDown)    state_transition(State::Action, 1u, &animations.TiltDown, 0u, nullptr);
    CASE (TiltForward) state_transition(State::Action, 1u, &animations.TiltForward, 0u, nullptr);
    CASE (TiltUp)      state_transition(State::Action, 1u, &animations.TiltUp, 0u, nullptr);

    CASE (AirBack)    state_transition(State::AirAction, 1u, &animations.AirBack, 0u, nullptr);
    CASE (AirDown)    state_transition(State::AirAction, 1u, &animations.AirDown, 0u, nullptr);
    CASE (AirForward) state_transition(State::AirAction, 1u, &animations.AirForward, 0u, nullptr);
    CASE (AirNeutral) state_transition(State::AirAction, 1u, &animations.AirNeutral, 0u, nullptr);
    CASE (AirUp)      state_transition(State::AirAction, 1u, &animations.AirUp, 0u, nullptr);

    CASE (DashAttack) state_transition(State::Action, 1u, &animations.DashAttack, 0u, nullptr);

    CASE (SmashDown)    state_transition(State::Charge, 1u, &animations.SmashDownStart, 0u, &animations.SmashDownCharge);
    CASE (SmashForward) state_transition(State::Charge, 1u, &animations.SmashForwardStart, 0u, &animations.SmashForwardCharge);
    CASE (SmashUp)      state_transition(State::Charge, 1u, &animations.SmashUpStart, 0u, &animations.SmashUpCharge);

    CASE (SpecialDown)    {} // todo
    CASE (SpecialForward) {} // todo
    CASE (SpecialNeutral) {} // todo
    CASE (SpecialUp)      {} // todo

    CASE (EvadeBack)    state_transition(State::Action, 1u, &animations.EvadeBack, 0u, nullptr);
    CASE (EvadeForward) state_transition(State::Action, 1u, &animations.EvadeForward, 0u, nullptr);
    CASE (Dodge)        state_transition(State::Action, 1u, &animations.Dodge, 0u, nullptr);

    CASE (AirDodge) state_transition(State::AirAction, 1u, &animations.AirDodge, 0u, nullptr);

    //--------------------------------------------------------//

    CASE ( None )
    {
        // we know here that current action is not nullptr

        SWITCH ( fighter.current.action->get_type() ) {

        CASE ( NeutralFirst, TiltForward, TiltUp, SmashDown, SmashForward, SmashUp, DashAttack )
        state_transition(State::Neutral, 0u, nullptr, 0u, &animations.NeutralLoop );

        CASE ( TiltDown )
        state_transition(State::Crouch, 0u, nullptr, 0u, &animations.CrouchLoop );

        CASE ( AirBack, AirDown, AirForward, AirNeutral, AirUp )
        state_transition(State::Jumping, 0u, nullptr, 0u, &animations.FallingLoop );

        CASE ( SpecialDown, SpecialForward, SpecialNeutral, SpecialUp )
        state_transition(State::Neutral, 0u, nullptr, 0u, &animations.NeutralLoop ); // todo

        CASE ( EvadeBack, EvadeForward, Dodge )
        state_transition(State::Neutral, 0u, nullptr, 0u, &animations.NeutralLoop );

        CASE ( AirDodge )
        state_transition(State::Jumping, 0u, nullptr, 0u, &animations.FallingLoop ); // todo

        CASE ( None ) SQASSERT(false, "switch from None to None");

        } SWITCH_END;
    }

    //--------------------------------------------------------//

    } SWITCH_END;

    //--------------------------------------------------------//

    if (fighter.current.action != nullptr)
        fighter.current.action->do_cancel();

    fighter.current.action = newAction;

    if (fighter.current.action != nullptr)
        if (fighter.current.state != State::Charge)
            fighter.current.action->do_start();

}

//============================================================================//

void PrivateFighter::update_commands(const Controller::Input& input)
{
    for (int i = 7; i != 0; --i)
        fighter.mCommands[i] = fighter.mCommands[i-1];

    fighter.mCommands[0].clear();

    //--------------------------------------------------------//

    using Command = Fighter::Command;
    Vector<Command>& cmds = fighter.mCommands[0];

    //--------------------------------------------------------//

    if (input.press_shield == true)
        cmds.push_back(Command::Shield);

    if (input.press_jump == true)
        cmds.push_back(Command::Jump);

    if (fighter.current.facing == +1 && input.norm_axis.x == -1)
        cmds.push_back(Command::TurnLeft);

    if (fighter.current.facing == -1 && input.norm_axis.x == +1)
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

void PrivateFighter::update_transitions(const Controller::Input& input)
{
    const Fighter::Stats& stats = fighter.stats;

    State& state = fighter.current.state;
    int8_t& facing = fighter.current.facing;

    Vec2F& velocity = fighter.mVelocity;
    Vec2F& translate = fighter.mTranslate;

    Stage& stage = get_world().get_stage();

    using Command = Fighter::Command;

    //--------------------------------------------------------//

    const auto try_catch_ledge = [&]() -> bool
    {
        SQASSERT(fighter.status.ledge == nullptr, "");

        if (mTimeSinceLedge <= STS_NO_LEDGE_CATCH_TIME)
            return false;

        // todo: unsure if position should be fighter's origin, or the centre of it's diamond
        fighter.status.ledge = stage.find_ledge(current.position, input.norm_axis.x);

        if (fighter.status.ledge == nullptr)
            return false;

        mTimeSinceLedge = 0u;

        // steal the ledge from some other fighter
        if (fighter.status.ledge->grabber != nullptr)
            fighter.status.ledge->grabber->status.ledge = nullptr;

        fighter.status.ledge->grabber = &fighter;
        facing = -fighter.status.ledge->direction;

        return true;
    };

    //--------------------------------------------------------//

    const auto state_transition_prejump = [&]()
    {
        state_transition(State::PreJump, 1u, &animations.PreJump, 0u, nullptr);

        mExtraJumps = stats.extra_jumps;
        mJumpHeld = true;
    };

    const auto do_animated_facing_change = [&]()
    {
        mAnimChangeFacing = true;
        facing = -facing;
    };

    //--------------------------------------------------------//

    // This is the heart of the fighter state machine. Each tick this should do
    // either zero or one state transition, not more. Things not coinciding with
    // these transitions should be done elsewhere.

    SWITCH ( state ) {

    CASE ( Neutral ) //=======================================//
    {
        if (fighter.consume_command(Command::Shield))
            state_transition(State::Shield, 2u, &animations.ShieldOn, 2u, &animations.ShieldLoop);

        else if (fighter.consume_command(Command::Jump))
            state_transition_prejump();

        else if (fighter.consume_command(Command::SmashDown))
            switch_action(ActionType::SmashDown);

        else if (fighter.consume_command(Command::SmashUp))
            switch_action(ActionType::SmashUp);

        else if (fighter.consume_command_facing(Command::SmashLeft, Command::SmashRight))
            switch_action(ActionType::SmashForward);

        else if (fighter.consume_command(Command::AttackDown))
            switch_action(ActionType::TiltDown);

        else if (fighter.consume_command(Command::AttackUp))
            switch_action(ActionType::TiltUp);

        else if (fighter.consume_command_facing(Command::AttackLeft, Command::AttackRight))
            switch_action(ActionType::TiltForward);

        else if (fighter.consume_command(Command::AttackNeutral))
            switch_action(ActionType::NeutralFirst);

        else if (fighter.consume_command(Command::MashDown))
            current.position.y -= 0.1f;

        else if (fighter.consume_command_facing(Command::TurnRight, Command::TurnLeft))
        {
            state_transition(State::Neutral, 0u, &animations.Turn, 0u, &animations.NeutralLoop);
            do_animated_facing_change();
        }

        else if (input.norm_axis.x == int8_t(facing))
        {
            if (mAnimation == &animations.Turn)
                state_transition(State::Walking, 0u, nullptr, 4u, &animations.WalkingLoop);
            else if (mVertigoActive == false)
                state_transition(State::Walking, 4u, &animations.WalkingLoop, 0u, nullptr);
            else state = State::Walking;
        }

        else if (input.int_axis.y == -2)
            state_transition(State::Crouch, 2u, &animations.CrouchOn, 0u, &animations.CrouchLoop);
    }

    CASE ( Walking ) //=======================================//
    {
        if (fighter.consume_command(Command::Shield))
            state_transition(State::Shield, 2u, &animations.ShieldOn, 0u, &animations.ShieldLoop);

        else if (fighter.consume_command(Command::Jump))
            state_transition_prejump();

        else if (fighter.consume_command(Command::SmashDown))
            switch_action(ActionType::SmashDown);

        else if (fighter.consume_command(Command::SmashUp))
            switch_action(ActionType::SmashUp);

        else if (fighter.consume_command_facing(Command::SmashLeft, Command::SmashRight))
            switch_action(ActionType::SmashForward);

        else if (fighter.consume_command(Command::AttackDown))
            switch_action(ActionType::TiltDown);

        else if (fighter.consume_command(Command::AttackUp))
            switch_action(ActionType::TiltUp);

        else if (fighter.consume_command_facing(Command::AttackLeft, Command::AttackRight))
            switch_action(ActionType::TiltForward);

        else if (fighter.consume_command_facing(Command::MashLeft, Command::MashRight))
            state_transition(State::Dashing, 0u, &animations.DashStart, 0u, nullptr);

        else if (input.int_axis.x == 0)
        {
            if (mAnimation == &animations.Turn)
                state_transition(State::Neutral, 0u, nullptr, 0u, &animations.NeutralLoop);
            else if (mVertigoActive == false)
                state_transition(State::Neutral, 4u, &animations.NeutralLoop, 0u, nullptr);
            else state = State::Neutral;
        }

        else if (input.int_axis.y == -2)
            state_transition(State::Crouch, 2u, &animations.CrouchOn, 0u, &animations.CrouchLoop);
    }

    CASE ( Dashing ) //=======================================//
    {
        // currently, dash start is the same state and speed as dashing
        // this may change in the future to better match smash bros
        //
        // also, currently not designed with right stick smashes in mind, so
        // dashing then doing a back smash without braking first won't work

        if (fighter.consume_command(Command::Shield))
            state_transition(State::Shield, 2u, &animations.ShieldOn, 0u, &animations.ShieldLoop);

        else if (fighter.consume_command(Command::Jump))
            state_transition_prejump();

        else if (fighter.consume_command_facing(Command::SmashLeft, Command::SmashRight))
            switch_action(ActionType::SmashForward);

        else if (fighter.consume_command_facing(Command::AttackLeft, Command::AttackRight))
            switch_action(ActionType::DashAttack);

        else if (input.int_axis.x != int8_t(facing) * 2)
        {
            if (mAnimation == &animations.DashStart)
                state_transition(State::Neutral, 0u, nullptr, 0u, &animations.NeutralLoop);
            else
                state_transition(State::Brake, 4u, &animations.Brake, 0u, nullptr);
        }

        // this shouldn't need a fade, but mario's animation doesn't line up
        else if (mAnimation == &animations.DashStart && mStateProgress == stats.dash_start_time)
            state_transition(State::Dashing, 2u, &animations.DashingLoop, 0u, nullptr);
    }

    CASE ( Brake ) //=========================================//
    {
        // why can't we shield while braking? needs testing

        if (fighter.consume_command(Command::Jump))
            state_transition_prejump();

        else if (fighter.consume_command(Command::SmashDown))
            switch_action(ActionType::SmashDown);

        else if (fighter.consume_command(Command::SmashUp))
            switch_action(ActionType::SmashUp);

        else if (fighter.consume_command_facing(Command::SmashLeft, Command::SmashRight))
            switch_action(ActionType::SmashForward);

        else if (fighter.consume_command_oldest({Command::AttackDown, Command::AttackUp,
                                                 Command::AttackLeft, Command::AttackRight,
                                                 Command::AttackNeutral}))
            switch_action(ActionType::DashAttack);

        else if (mAnimation == &animations.Brake)
        {
            if (fighter.consume_command_facing(Command::TurnRight, Command::TurnLeft))
            {
                state_transition(State::Brake, 0u, &animations.TurnDash, 0u, nullptr);
                do_animated_facing_change();
            }

            else if (mStateProgress == stats.dash_brake_time)
                state_transition(State::Neutral, 0u, nullptr, 0u, &animations.NeutralLoop);
        }

        else if (mStateProgress == stats.dash_turn_time)
        {
            SQASSERT(mAnimation == &animations.TurnDash, "");

            if (input.int_axis.x == int8_t(facing) * 2)
                state_transition(State::Dashing, 0u, nullptr, 0u, &animations.DashingLoop);
            else
                state_transition(State::Neutral, 0u, &animations.TurnBrake, 0u, &animations.NeutralLoop);
        }
    }

    CASE ( Crouch ) //========================================//
    {
        // we only handle down smashes and tilts in the crouch state
        // inputing any other attack will also cause the crouch state to
        // end, so the new state can do the attack next frame

        if (fighter.consume_command(Command::Shield))
            state_transition(State::Shield, 2u, &animations.ShieldOn, 0u, &animations.ShieldLoop);

        else if (fighter.consume_command(Command::Jump))
            state_transition_prejump();

        else if (fighter.consume_command(Command::SmashDown))
            switch_action(ActionType::SmashDown);

        else if (fighter.consume_command(Command::AttackDown))
            switch_action(ActionType::TiltDown);

        else if (input.int_axis.y != -2)
            state_transition(State::Neutral, 2u, &animations.CrouchOff, 0u, &animations.NeutralLoop);
    }

    CASE ( PreJump ) //=======================================//
    {
        if (mStateProgress == STS_JUMP_DELAY)
        {
            if (input.norm_axis.x == -facing)
                state_transition(State::Jumping, 1u, &animations.JumpBack, 0u, &animations.FallingLoop);
            else
                state_transition(State::Jumping, 1u, &animations.JumpForward, 0u, &animations.FallingLoop);

            // this could be a seperate stat, but half air speed seems good enough
            velocity.x = stats.air_speed * input.float_axis.x * 0.5f;

            const float height = mJumpHeld ? stats.jump_height : stats.hop_height;
            velocity.y = std::sqrt(2.f * height * stats.gravity) + stats.gravity * 0.5f;
        }
    }

    CASE ( Landing ) //=======================================//
    {
        if (mStateProgress == mLandingLag)
            state_transition(State::Neutral, 0u, nullptr, 0u, &animations.NeutralLoop);
    }

    CASE ( Jumping ) //=======================================//
    {
        if (try_catch_ledge() == true)
            state_transition(State::LedgeHang, 1u, &animations.LedgeCatch, 0u, &animations.LedgeLoop);

        else if (fighter.consume_command(Command::Shield))
            switch_action(ActionType::AirDodge);

        else if (mExtraJumps > 0u && fighter.consume_command(Command::Jump))
        {
            if (input.norm_axis.x == -facing)
                state_transition(State::Jumping, 2u, &animations.AirHopBack, 1u, &animations.FallingLoop);
            else
                state_transition(State::Jumping, 2u, &animations.AirHopForward, 1u, &animations.FallingLoop);

            mExtraJumps -= 1u;
            velocity.y = std::sqrt(2.f * stats.gravity * stats.airhop_height) + stats.gravity * 0.5f;
        }

        else if (fighter.consume_command_oldest({Command::SmashDown, Command::AttackDown}))
            switch_action(ActionType::AirDown);

        else if (fighter.consume_command_oldest({Command::SmashUp, Command::AttackUp}))
            switch_action(ActionType::AirUp);

        else if (fighter.consume_command_oldest_facing({Command::SmashLeft, Command::AttackLeft},
                                                       {Command::SmashRight, Command::AttackRight}))
            switch_action(ActionType::AirForward);

        else if (fighter.consume_command_oldest_facing({Command::SmashRight, Command::AttackRight},
                                                       {Command::SmashLeft, Command::AttackLeft}))
            switch_action(ActionType::AirBack);

        else if (fighter.consume_command(Command::AttackNeutral))
            switch_action(ActionType::AirNeutral);
    }

    CASE ( Shield ) //========================================//
    {
        if (fighter.consume_command(Command::Jump))
            state_transition_prejump();

        else if (fighter.consume_command_facing(Command::MashLeft, Command::MashRight))
        {
            switch_action(ActionType::EvadeForward);
            do_animated_facing_change();
        }

        else if (fighter.consume_command_facing(Command::MashRight, Command::MashLeft))
            switch_action(ActionType::EvadeBack);

        else if (fighter.consume_command_oldest({Command::MashDown, Command::MashUp}))
            switch_action(ActionType::Dodge);

        else if (input.hold_shield == false)
            state_transition(State::Neutral, 2u, &animations.ShieldOff, 0u, &animations.NeutralLoop);
    }

    CASE ( LedgeHang ) //=====================================//
    {
        // someone else stole our ledge on the previous frame
        if (fighter.status.ledge == nullptr)
        {
            // todo: this needs to be handled later, to give p2 a chance to steal p1's ledge
            state_transition(State::Jumping, 0u, &animations.FallingLoop, 0u, nullptr);
            translate.x -= fighter.diamond.halfWidth * float(facing);
            translate.y -= fighter.diamond.offsetTop * 0.75f;
            mExtraJumps = stats.extra_jumps;
        }

        else if (mStateProgress >= STS_MIN_LEDGE_HANG_TIME)
        {
            if (fighter.consume_command(Command::Jump))
            {
                state_transition(State::Jumping, 1u, &animations.LedgeJump, 0u, &animations.FallingLoop);
                velocity.y = std::sqrt(2.f * stats.jump_height * stats.gravity) + stats.gravity * 0.5f;
                fighter.status.ledge->grabber = nullptr;
                fighter.status.ledge = nullptr;
                mExtraJumps = stats.extra_jumps;
            }

            else if (fighter.consume_command(Command::MashUp) ||
                     fighter.consume_command_facing(Command::MashLeft, Command::MashRight))
            {
                state_transition(State::LedgeClimb, 1u, &animations.LedgeClimb, 0u, nullptr);
                fighter.status.ledge->grabber = nullptr;
                fighter.status.ledge = nullptr;
            }

            else if (fighter.consume_command(Command::MashDown) ||
                     fighter.consume_command_facing(Command::MashRight, Command::MashLeft))
            {
                state_transition(State::Jumping, 0u, &animations.FallingLoop, 0u, nullptr);
                fighter.status.ledge->grabber = nullptr;
                fighter.status.ledge = nullptr;
                translate.x -= fighter.diamond.halfWidth * float(facing);
                translate.y -= fighter.diamond.offsetTop * 0.75f;
                mExtraJumps = stats.extra_jumps;
            }
        }
    }

    CASE ( LedgeClimb ) //====================================//
    {
        if (mStateProgress == stats.ledge_climb_time)
            state_transition(State::Neutral, 0u, nullptr, 0u, &animations.NeutralLoop);
    }

    CASE ( Charge ) //========================================//
    {
        SQASSERT(fighter.current.action != nullptr, "can't charge null ptrs");

        if (input.hold_attack == false) // todo: or max charge reached
        {
            if (fighter.current.action->get_type() == ActionType::SmashDown)
                state_transition(State::Action, 1u, &animations.SmashDownAttack, 0u, nullptr);

            else if (fighter.current.action->get_type() == ActionType::SmashForward)
                state_transition(State::Action, 1u, &animations.SmashForwardAttack, 0u, nullptr);

            else if (fighter.current.action->get_type() == ActionType::SmashUp)
                state_transition(State::Action, 1u, &animations.SmashUpAttack, 0u, nullptr);

            else SQASSERT(false, "only smash attacks can be charged");

            fighter.current.action->do_start();
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

void PrivateFighter::update_states(const Controller::Input& input)
{
    const Fighter::Stats& stats = fighter.stats;

    State& state = fighter.current.state;
    int8_t& facing = fighter.current.facing;

    Stage& stage = get_world().get_stage();

    mStateProgress += 1u;

    //-- misc non-transition state updates -------------------//

    if (state == State::PreJump)
        if (input.hold_jump == false)
            mJumpHeld = false;

    //-- most updates don't apply when ledge hanging ---------//

    if (state == State::LedgeHang)
    {
        SQASSERT(fighter.status.ledge != nullptr, "nope");

        // this will be the case only if we just grabbed the ledge
        if (fighter.mVelocity != Vec2F())
        {
            current.position = (current.position + fighter.status.ledge->position) / 2.f;
            fighter.mVelocity = Vec2F();
        }
        else current.position = fighter.status.ledge->position;

        return; // EARLY RETURN
    }

    mTimeSinceLedge += 1u;

    //-- apply friction --------------------------------------//

    SWITCH ( state ) {

    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Charge, Action, Landing, Shield )
    {
        if (fighter.mVelocity.x < -0.f) fighter.mVelocity.x = maths::min(fighter.mVelocity.x + stats.traction, -0.f);
        if (fighter.mVelocity.x > +0.f) fighter.mVelocity.x = maths::max(fighter.mVelocity.x - stats.traction, +0.f);
    }

    CASE ( Jumping, Helpless, AirAction )
    {
        if (input.int_axis.x == 0)
        {
            if (fighter.mVelocity.x < -0.f) fighter.mVelocity.x = maths::min(fighter.mVelocity.x + stats.air_friction, -0.f);
            if (fighter.mVelocity.x > +0.f) fighter.mVelocity.x = maths::max(fighter.mVelocity.x - stats.air_friction, +0.f);
        }
    }

    CASE ( PreJump, Knocked, Stunned, LedgeHang, LedgeClimb, EditorPreview ) {}

    } SWITCH_END;

    //-- add horizontal velocity -----------------------------//

    const auto apply_horizontal_move = [&](float mobility, float speed)
    {
        if (input.int_axis.x < 0 && fighter.mVelocity.x <= input.float_axis.x * speed) return;
        if (input.int_axis.x > 0 && fighter.mVelocity.x >= input.float_axis.x * speed) return;

        fighter.mVelocity.x += float(input.norm_axis.x) * mobility;
        fighter.mVelocity.x = maths::clamp_magnitude(fighter.mVelocity.x, input.float_axis.x * speed);
    };

    if (input.int_axis.x != 0)
    {
        if (state == State::Jumping || state == State::Helpless)
            apply_horizontal_move(stats.air_mobility, stats.air_speed);

        else if (state == State::AirAction)
            apply_horizontal_move(stats.air_mobility, stats.air_speed);

        else if (state == State::Walking)
            apply_horizontal_move(stats.traction * 2.f, stats.walk_speed);

        else if (state == State::Dashing)
            apply_horizontal_move(stats.traction * 4.f, stats.dash_speed);
    }

    //-- apply gravity, velocity, and translation ------------//

    fighter.mVelocity.y = maths::max(fighter.mVelocity.y - stats.gravity, -stats.fall_speed);

    const Vec2F translation = fighter.mVelocity + fighter.mTranslate;
    const Vec2F targetPosition = current.position + translation;

    //-- ask the stage where we can move ---------------------//

    const bool edgeStop = ( state == State::Neutral || state == State::Shield || state == State::Landing ||
                            state == State::PreJump || state == State::Action || state == State::Charge ) ||
                          ( ( state == State::Walking || state == State::Dashing || state == State::Brake ) &&
                            !( input.int_axis.x == -2 && translation.x < -0.0f ) &&
                            !( input.int_axis.x == +2 && translation.x > +0.0f ) );

    MoveAttempt moveAttempt = stage.attempt_move(fighter.diamond, current.position, targetPosition, edgeStop);

    current.position = moveAttempt.result;

    fighter.mTranslate = Vec2F();

    //-- activate vertigo animation --------------------------//

    if (state == State::Neutral || state == State::Walking)
    {
        if (mVertigoActive == false && moveAttempt.edge == int8_t(facing))
        {
            state_transition(state, 2u, &animations.VertigoStart, 0u, &animations.VertigoLoop);
            mVertigoActive = true;
        }
    }
    else mVertigoActive = false;

    //-- handle starting to fall and landing -----------------//

    SWITCH (state) {

    // normal air states, can do light or heavy landing
    CASE ( Jumping, Helpless )
    {
        if (moveAttempt.collideFloor == true)
        {
            const bool light = fighter.mVelocity.y > -stats.fall_speed || mStateProgress < stats.land_heavy_min_time;

            state_transition(State::Landing, 1u, light ? &animations.LandLight : &animations.LandHeavy, 0u, nullptr);
            mLandingLag = light ? STS_LIGHT_LANDING_LAG : STS_HEAVY_LANDING_LAG;

            fighter.mVelocity.y = 0.f;
        }
    }

    // air actions all have different amounts of landing lag
    CASE ( AirAction )
    {
        if (moveAttempt.collideFloor == true)
        {
            switch_action(ActionType::None);

            state_transition(State::Landing, 1u, &animations.LandHeavy, 0u, nullptr);
            mLandingLag = STS_HEAVY_LANDING_LAG * 2u; // todo

            fighter.mVelocity.y = 0.f;
        }
    }

    // landing while knocked causes you to bounce and be stuck for a bit
    CASE ( Knocked )
    {
        if (moveAttempt.collideFloor == true)
        {
            if (maths::length(fighter.mVelocity) > 5.f)
            {
                // todo
            }
            state_transition(State::Landing, 1u, &animations.LandHeavy, 0u, nullptr);
            mLandingLag = STS_HEAVY_LANDING_LAG;

            fighter.mVelocity.y = 0.f;
        }
    }

    // these ground states have a special dive animation
    CASE ( Walking, Dashing )
    {
        if (moveAttempt.collideFloor == false)
        {
            // todo
            state_transition(State::Jumping, 1u, &animations.FallingLoop, 0u, nullptr);

            mExtraJumps = stats.extra_jumps;
        }
        else fighter.mVelocity.y = 0.f;
    }

    // misc ground states that you can fall from
    CASE ( Neutral, Action, Brake, Crouch, Charge, Landing, Stunned, Shield )
    {
        if (moveAttempt.collideFloor == false)
        {
            state_transition(State::Jumping, 2u, &animations.FallingLoop, 0u, nullptr);

            mExtraJumps = stats.extra_jumps; // todo: is this correct for Action?
        }
        else fighter.mVelocity.y = 0.f;
    }

    // special states that you can't fall or land from
    CASE ( PreJump, LedgeHang, LedgeClimb, EditorPreview ) {}

    } SWITCH_END;

    //-- check if we've hit a ceiling or wall ----------------//

    // todo: this should cause our fighter to bounce
    if (state == State::Knocked)
    {
        if (moveAttempt.collideCeiling == true)
            fighter.mVelocity.y = 0.f;

        if (moveAttempt.collideWall == true)
            fighter.mVelocity.x = 0.f;
    }

    // todo: this should probably have some animations
    else
    {
        if (moveAttempt.collideCeiling == true)
            fighter.mVelocity.y = 0.f;

        if (moveAttempt.collideWall == true)
            fighter.mVelocity.x = 0.f;
    }

    //-- update the active action ----------------------------//

    if (fighter.current.action != nullptr && fighter.current.state != State::Charge)
        if (fighter.current.action->do_tick() == true)
            switch_action(ActionType::None);
}

//============================================================================//

void PrivateFighter::base_tick_fighter()
{
    fighter.previous = fighter.current;

    previous = current;

    //--------------------------------------------------------//

    if (get_world().globals.editorMode == true)
    {
        const auto input = Controller::Input();

            update_commands(input);
            update_transitions(input);
            update_states(input);
    }
    else
    {
        SQASSERT(controller != nullptr, "");
        const auto input = controller->get_input();

        update_commands(input);
        update_transitions(input);
        update_states(input);
    }

    //--------------------------------------------------------//

    const int8_t rotFacing = fighter.current.facing * (mAnimChangeFacing ? -1 : +1);
    const float rotY = fighter.current.state == State::EditorPreview ? 0.5f : 0.25f * float(rotFacing);

    current.rotation = QuatF(0.f, rotY, 0.f);

    update_animation();

    armature.compute_ubo_data(current.pose, fighter.mBoneMatrices.data(), fighter.mBoneMatrices.size());

    fighter.mModelMatrix = maths::transform(Vec3F(current.position, 0.f), current.rotation, Vec3F(1.f));
}

//============================================================================//

void PrivateFighter::update_animation()
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
    const QuatF rootInverseRestRotation = maths::inverse(armature.get_rest_pose().front().rotation);

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
        current.pose = armature.compute_pose_discrete(anim.anim, time);
        current.rotation = root().rotation * rootInverseRestRotation * current.rotation;
        root().rotation = armature.get_rest_pose().front().rotation;

        sq::log_info("pose: %s - %d", anim.key, time);

        mPreviousAnimation = &anim;
        mPreviousAnimTimeDiscrete = time;
    };

    const auto set_current_pose_continuous = [&](const Animation& anim, float time)
    {
        current.pose = armature.compute_pose_continuous(anim.anim, time);
        current.rotation = root().rotation * rootInverseRestRotation * current.rotation;
        root().rotation = armature.get_rest_pose().front().rotation;

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

            const float distance = std::abs(fighter.mVelocity.x);
            const float animSpeed = fighter.stats.anim_walk_stride / float(mAnimation->anim.totalTime);

            mAnimTimeContinuous += distance / animSpeed;

            set_current_pose_continuous(*mAnimation, mAnimTimeContinuous);

            root().offset = Vec3F();
        }

        CASE (DashCycle) //=======================================//
        {
            SQASSERT(mAnimation->anim.looping() == true, "");
            SQASSERT(mNextAnimation == nullptr, "");

            const float distance = std::abs(fighter.mVelocity.x);
            const float length = fighter.stats.anim_dash_stride / float(mAnimation->anim.totalTime);

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

            fighter.mTranslate = { mAnimChangeFacing ? -offsetLocal.z : offsetLocal.z, offsetLocal.x };
            fighter.mTranslate.x *= float(fighter.current.facing);

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
        current.pose = armature.blend_poses(mFadeStartPose, current.pose, blend);
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
