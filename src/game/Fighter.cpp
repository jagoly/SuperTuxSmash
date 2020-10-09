#include "game/Fighter.hpp"

#include "main/Options.hpp"

#include "game/Action.hpp"
#include "game/FightWorld.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

Fighter::Fighter(FightWorld& world, FighterEnum type, uint8_t index)
    : world(world), type(type), index(index)
    , mHurtBlobs(world.get_memory_resource())
{
    const String path = sq::build_string("assets/fighters/", sq::enum_to_string(type));

    initialise_attributes(path);
    initialise_armature(path);
    initialise_hurtblobs(path);

    for (int8_t i = 0; i < sq::enum_count_v<ActionType>; ++i)
        mActions[i] = std::make_unique<Action>(*this, ActionType(i));
}

Fighter::~Fighter() = default;

//============================================================================//

void Fighter::initialise_attributes(const String& path)
{
    const auto json = sq::parse_json_from_file(path + "/Attributes.json");

    SQASSERT(json.size() >= 23+1, "not enough attributes in json");
    SQASSERT(json.size() <= 23+1, "too many attributes in json");

    attributes.walk_speed     = json.at("walk_speed");
    attributes.dash_speed     = json.at("dash_speed");
    attributes.air_speed      = json.at("air_speed");
    attributes.traction       = json.at("traction");
    attributes.air_mobility   = json.at("air_mobility");
    attributes.air_friction   = json.at("air_friction");
    attributes.hop_height     = json.at("hop_height");
    attributes.jump_height    = json.at("jump_height");
    attributes.airhop_height  = json.at("airhop_height");
    attributes.gravity        = json.at("gravity");
    attributes.fall_speed     = json.at("fall_speed");
    attributes.fastfall_speed = json.at("fastfall_speed");
    attributes.weight         = json.at("weight");

    attributes.extra_jumps = json.at("extra_jumps");

    attributes.land_heavy_fall_time = json.at("land_heavy_fall_time");

    attributes.dash_start_time  = json.at("dash_start_time");
    attributes.dash_brake_time  = json.at("dash_brake_time");
    attributes.dash_turn_time   = json.at("dash_turn_time");

    attributes.anim_walk_stride = json.at("anim_walk_stride");
    attributes.anim_dash_stride = json.at("anim_dash_stride");

    mLocalDiamond.halfWidth   = json.at("diamond_half_width");
    mLocalDiamond.offsetCross = json.at("diamond_offset_cross");
    mLocalDiamond.offsetTop   = json.at("diamond_offset_top");

    mLocalDiamond.compute_normals();
}

//============================================================================//

void Fighter::initialise_armature(const String& path)
{
    mArmature.load_from_file(path + "/Armature.json");

    current.pose = previous.pose = mArmature.get_rest_pose();

    mBoneMatrices.resize(mArmature.get_bone_count());

    const auto load_anim = [&](const char* name, AnimMode mode) -> Animation
    {
        const String filePath = sq::build_string(path, "/anims/", name, ".sqa");
        if (sq::check_file_exists(filePath) == false)
        {
            sq::log_warning("missing animation '{}'", filePath);
            return { mArmature.make_null_animation(1u), mode, name };
        }
        return { mArmature.make_animation(filePath), mode, name };
    };

    Animations& anims = mAnimations;

    anims.DashingLoop = load_anim("DashingLoop", AnimMode::DashCycle);
    anims.FallingLoop = load_anim("FallingLoop", AnimMode::Looping);
    anims.NeutralLoop = load_anim("NeutralLoop", AnimMode::Looping);
    anims.ProneLoop = load_anim("ProneLoop", AnimMode::Looping);
    anims.TumbleLoop = load_anim("TumbleLoop", AnimMode::Looping);
    anims.VertigoLoop = load_anim("VertigoLoop", AnimMode::Looping);
    anims.WalkingLoop = load_anim("WalkingLoop", AnimMode::WalkCycle);

    anims.ShieldOn = load_anim("ShieldOn", AnimMode::Standard);
    anims.ShieldOff = load_anim("ShieldOff", AnimMode::Standard);
    anims.ShieldLoop = load_anim("ShieldLoop", AnimMode::Looping);

    anims.CrouchOn = load_anim("CrouchOn", AnimMode::Standard);
    anims.CrouchOff = load_anim("CrouchOff", AnimMode::Standard);
    anims.CrouchLoop = load_anim("CrouchLoop", AnimMode::Looping);

    anims.DashStart = load_anim("DashStart", AnimMode::Standard);
    anims.VertigoStart = load_anim("VertigoStart", AnimMode::Standard);

    anims.Brake = load_anim("Brake", AnimMode::Standard);
    anims.PreJump = load_anim("PreJump", AnimMode::Standard);
    anims.Turn = load_anim("Turn", AnimMode::ApplyTurn);
    anims.TurnBrake = load_anim("TurnBrake", AnimMode::Standard);
    anims.TurnDash = load_anim("TurnDash", AnimMode::ApplyTurn);

    anims.JumpBack = load_anim("JumpBack", AnimMode::Standard);
    anims.JumpForward = load_anim("JumpForward", AnimMode::Standard);
    anims.AirHopBack = load_anim("AirHopBack", AnimMode::Standard);
    anims.AirHopForward = load_anim("AirHopForward", AnimMode::Standard);

    anims.LedgeCatch = load_anim("LedgeCatch", AnimMode::Standard);
    anims.LedgeLoop = load_anim("LedgeLoop", AnimMode::Looping);
    anims.LedgeClimb = load_anim("LedgeClimb", AnimMode::ApplyMotion);
    anims.LedgeJump = load_anim("LedgeJump", AnimMode::Standard);

    anims.NeutralFirst = load_anim("NeutralFirst", AnimMode::ApplyMotion);
    anims.NeutralSecond = load_anim("NeutralSecond", AnimMode::ApplyMotion);
    anims.NeutralThird = load_anim("NeutralThird", AnimMode::ApplyMotion);

    anims.DashAttack = load_anim("DashAttack", AnimMode::ApplyMotion);

    anims.TiltDown = load_anim("TiltDown", AnimMode::ApplyMotion);
    anims.TiltForward = load_anim("TiltForward", AnimMode::ApplyMotion);
    anims.TiltUp = load_anim("TiltUp", AnimMode::ApplyMotion);

    anims.EvadeBack = load_anim("EvadeBack", AnimMode::ApplyMotion);
    anims.EvadeForward = load_anim("EvadeForward", AnimMode::MotionTurn);
    anims.Dodge = load_anim("Dodge", AnimMode::ApplyMotion);

    anims.ProneAttack = load_anim("ProneAttack", AnimMode::ApplyMotion);
    anims.ProneBack = load_anim("ProneBack", AnimMode::ApplyMotion);
    anims.ProneForward = load_anim("ProneForward", AnimMode::ApplyMotion);
    anims.ProneStand = load_anim("ProneStand", AnimMode::ApplyMotion);

    anims.SmashDownStart = load_anim("SmashDownStart", AnimMode::ApplyMotion);
    anims.SmashForwardStart = load_anim("SmashForwardStart", AnimMode::ApplyMotion);
    anims.SmashUpStart = load_anim("SmashUpStart", AnimMode::ApplyMotion);

    anims.SmashDownCharge = load_anim("SmashDownCharge", AnimMode::Looping);
    anims.SmashForwardCharge = load_anim("SmashForwardCharge", AnimMode::Looping);
    anims.SmashUpCharge = load_anim("SmashUpCharge", AnimMode::Looping);

    anims.SmashDownAttack = load_anim("SmashDownAttack", AnimMode::ApplyMotion);
    anims.SmashForwardAttack = load_anim("SmashForwardAttack", AnimMode::ApplyMotion);
    anims.SmashUpAttack = load_anim("SmashUpAttack", AnimMode::ApplyMotion);

    anims.AirBack = load_anim("AirBack", AnimMode::Standard);
    anims.AirDown = load_anim("AirDown", AnimMode::Standard);
    anims.AirForward = load_anim("AirForward", AnimMode::Standard);
    anims.AirNeutral = load_anim("AirNeutral", AnimMode::Standard);
    anims.AirUp = load_anim("AirUp", AnimMode::Standard);
    anims.AirDodge = load_anim("AirDodge", AnimMode::Standard);

    anims.LandLight = load_anim("LandLight", AnimMode::Standard);
    anims.LandHeavy = load_anim("LandHeavy", AnimMode::Standard);
    anims.LandTumble = load_anim("LandTumble", AnimMode::Standard);

    anims.LandAirBack = load_anim("LandAirBack", AnimMode::Standard);
    anims.LandAirDown = load_anim("LandAirDown", AnimMode::Standard);
    anims.LandAirForward = load_anim("LandAirForward", AnimMode::Standard);
    anims.LandAirNeutral = load_anim("LandAirNeutral", AnimMode::Standard);
    anims.LandAirUp = load_anim("LandAirUp", AnimMode::Standard);

    anims.HurtLowerLight = load_anim("HurtLowerLight", AnimMode::Standard);
    anims.HurtLowerHeavy = load_anim("HurtLowerHeavy", AnimMode::Standard);
    anims.HurtLowerTumble = load_anim("HurtLowerTumble", AnimMode::Standard);

    anims.HurtMiddleLight = load_anim("HurtMiddleLight", AnimMode::Standard);
    anims.HurtMiddleHeavy = load_anim("HurtMiddleHeavy", AnimMode::Standard);
    anims.HurtMiddleTumble = load_anim("HurtMiddleTumble", AnimMode::Standard);

    anims.HurtUpperLight = load_anim("HurtUpperLight", AnimMode::Standard);
    anims.HurtUpperHeavy = load_anim("HurtUpperHeavy", AnimMode::Standard);
    anims.HurtUpperTumble = load_anim("HurtUpperTumble", AnimMode::Standard);

    anims.HurtAirLight = load_anim("HurtAirLight", AnimMode::Standard);
    anims.HurtAirHeavy = load_anim("HurtAirHeavy", AnimMode::Standard);

    //anims.LaunchLoop = load_anim("LaunchLoop", AnimMode::Looping);
    //anims.LaunchFinish = load_anim("LaunchFinish", AnimMode::Standard);

    const auto ensure_anim_at_least = [](Animation& anim, uint time, const char* timeName)
    {
        if (anim.anim.frameCount > time) return; // anim is longer than time

        if (anim.anim.frameCount != 1u) // fallback animation, don't print another warning
            sq::log_error("anim '{}' shorter than '{}'", anim.key, timeName);

        anim.anim.frameCount = time + 1u;
        //anim.anim.poses.resize(time + 1u, anim.anim.poses.back());
    };

    ensure_anim_at_least(anims.DashStart, attributes.dash_start_time, "dash_start_time");
    ensure_anim_at_least(anims.Brake, attributes.dash_brake_time, "dash_brake_time");
    ensure_anim_at_least(anims.TurnDash, attributes.dash_turn_time, "dash_turn_time");

    ensure_anim_at_least(anims.HurtLowerHeavy, MIN_HITSTUN_HEAVY, "MIN_HITSTUN_HEAVY");
    ensure_anim_at_least(anims.HurtMiddleHeavy, MIN_HITSTUN_HEAVY, "MIN_HITSTUN_HEAVY");
    ensure_anim_at_least(anims.HurtUpperHeavy, MIN_HITSTUN_HEAVY, "MIN_HITSTUN_HEAVY");

    ensure_anim_at_least(anims.HurtLowerTumble, MIN_HITSTUN_TUMBLE, "MIN_HITSTUN_TUMBLE");
    ensure_anim_at_least(anims.HurtMiddleTumble, MIN_HITSTUN_TUMBLE, "MIN_HITSTUN_TUMBLE");
    ensure_anim_at_least(anims.HurtUpperTumble, MIN_HITSTUN_TUMBLE, "MIN_HITSTUN_TUMBLE");
}

//============================================================================//

void Fighter::initialise_hurtblobs(const String& path)
{
    const JsonValue root = sq::parse_json_from_file(path + "/HurtBlobs.json");
    for (const auto& item : root.items())
    {
        HurtBlob& blob = mHurtBlobs[item.key()];
        blob.fighter = this;

        try { blob.from_json(item.value()); }
        catch (const std::exception& e) {
            sq::log_warning("problem loading hurtblob '{}': {}", item.key(), e.what());
        }

        world.enable_hurtblob(&blob);
    }
}

//============================================================================//

bool Fighter::consume_command(Command cmd)
{
    for (size_t i = CMD_BUFFER_SIZE; i != 0u; --i)
    {
        for (size_t j = mCommands[i-1u].size(); j != 0u; --j)
        {
            if (mCommands[i-1u][j-1u] == cmd)
            {
                mCommands[i-1u][j-1u] = Command(uint8_t(cmd) | uint8_t(Command::CONSUMED));
                if (world.options.log_input == true)
                    sq::log_debug("consumed command: {}", cmd);
                return true;
            }
        }
    }
    return false;
}

bool Fighter::consume_command(std::initializer_list<Command> cmds)
{
    for (size_t i = CMD_BUFFER_SIZE; i != 0u; --i)
    {
        for (size_t j = mCommands[i-1u].size(); j != 0u; --j)
        {
            for (Command cmd : cmds)
            {
                if (mCommands[i-1u][j-1u] == cmd)
                {
                    mCommands[i-1u][j-1u] = Command(uint8_t(cmd) | uint8_t(Command::CONSUMED));
                    if (world.options.log_input == true)
                        sq::log_debug("consumed command: {}", cmd);
                    return true;
                }
            }
        }
    }
    return false;
}

bool Fighter::consume_command_facing(Command leftCmd, Command rightCmd)
{
    if (status.facing == -1) return consume_command(leftCmd);
    if (status.facing == +1) return consume_command(rightCmd);
    return false; // make compiler happy
}

bool Fighter::consume_command_facing(std::initializer_list<Command> leftCmds, std::initializer_list<Command> rightCmds)
{
    if (status.facing == -1) return consume_command(leftCmds);
    if (status.facing == +1) return consume_command(rightCmds);
    return false; // make compiler happy
}

//============================================================================//

void Fighter::apply_hit_generic(const HitBlob& hit, const HurtBlob& hurt)
{
    SQASSERT(&hit.action->fighter != this, "invalid hitblob");
    SQASSERT(hurt.fighter == this, "invalid hurtblob");

    // todo: individual regions/hurtblobs can become intangible
    if (status.intangible == true) return;

    // if we were in the middle of an action, cancel it
    cancel_active_action();

    //--------------------------------------------------------//

    // knockback formula: https://www.ssbwiki.com/Knockback#Melee_onward
    // freezetime formula: https://www.ssbwiki.com/Hitlag#Formula
    // knockback and weight values are the same arbitary units as in smash bros
    // hitstun should always end before launch speed completely decays
    // non-launching attacks can not cause tumbling

    const bool shielding = status.state == State::Shield || (status.state == State::Freeze && mFrozenState == State::Shield) ||
                           status.state == State::ShieldStun || (status.state == State::Freeze && mFrozenState == State::ShieldStun);

    const float facing = hit.facing == BlobFacing::Relative ? hit.action->fighter.status.position.x < status.position.x ? +1.f : -1.f :
                         hit.facing == BlobFacing::Forward ? float(hit.action->fighter.status.facing) : -float(hit.action->fighter.status.facing);

    const uint freezeTime = uint((hit.damage * 0.4f + 4.f) * hit.freezeFactor);

    //--------------------------------------------------------//

    if (shielding == true)
    {
        status.shield -= hit.damage;

        if (status.shield > 0.f)
        {
            status.velocity.x += facing * (SHIELD_PUSH_HURT_BASE + hit.damage * SHIELD_PUSH_HURT_FACTOR);
            hit.action->fighter.status.velocity.x -= facing * (SHIELD_PUSH_HIT_BASE + hit.damage * SHIELD_PUSH_HIT_FACTOR);

            mHitStunTime = uint(SHIELD_STUN_BASE + SHIELD_STUN_FACTOR * hit.damage);
            mFrozenState = State::ShieldStun;
        }
        else apply_shield_break(); // todo
    }

    //--------------------------------------------------------//

    else // not shielding
    {
        const bool airborne = status.state == State::JumpFall || (status.state == State::Freeze && mFrozenState == State::JumpFall) ||
                              status.state == State::TumbleFall || (status.state == State::Freeze && mFrozenState == State::TumbleFall) ||
                              status.state == State::AirStun || (status.state == State::Freeze && mFrozenState == State::AirStun) ||
                              status.state == State::AirAction || (status.state == State::Freeze && mFrozenState == State::AirAction);

        status.damage += hit.damage;

        const float damageFactor = status.damage / 10.f + (status.damage * hit.damage) / 20.f;
        const float weightFactor = 200.f / (attributes.weight + 100.f);

        const float knockbackNormal = hit.knockBase + (damageFactor * weightFactor * 1.4f + 18.f) * hit.knockScale * 0.01f;
        const float knockbackFixed = (1.f + 10.f * hit.knockBase / 20.f) * weightFactor * 1.4f + 18.f;
        const float knockback = hit.useFixedKnockback ? knockbackFixed : knockbackNormal;

        const float launchSpeed = knockback * 0.003f;
        const uint hitStunTime = uint(knockback * 0.4f);

        const float angleNormal = maths::radians(hit.knockAngle / 360.f);
        const float angleSakurai = maths::radians((airborne ? 45.f : hitStunTime > 16u ? 40.f : 0.f) / 360.f);
        const float angle = hit.useSakuraiAngle ? angleSakurai : angleNormal;

        const Vec2F knockDir = { std::cos(angle) * facing, std::sin(angle) };

        sq::log_debug_multiline("fighter {} hit by fighter {}:"
                                "\nknockback:   {}" "\nfreezeTime:  {}" "\nhitStun:     {}"
                                "\nknockDir:    {}" "\nlaunchSpeed: {}" "\ndecayFrames: {}"
                                "\nhitKey:      {}" "\nhurtKey:     {}" "\nhurtRegion:  {}",
                                index, hit.action->fighter.index,
                                knockback, freezeTime, hitStunTime,
                                knockDir, launchSpeed, uint(launchSpeed / KNOCKBACK_DECAY),
                                hit.get_key(), hurt.get_key(), hurt.region);

        SQASSERT(hitStunTime < uint(launchSpeed / KNOCKBACK_DECAY), "something is broken");

        mHitStunTime = hitStunTime;

        status.velocity = maths::normalize(knockDir) * launchSpeed;
        // todo: should be based on knockback dir (see back air for why this wrong)
        status.facing = -hit.action->fighter.status.facing;

        const auto& anims = mAnimations;

        if (airborne == true || angle != 0.f)
        {
            if (mHitStunTime >= MIN_HITSTUN_TUMBLE)
            {
                mFrozenState = State::TumbleStun;

                if (hurt.region == BlobRegion::Middle)
                    state_transition(State::Freeze, 0u, &anims.HurtMiddleTumble, 0u, &anims.TumbleLoop);
                else if (hurt.region == BlobRegion::Lower)
                    state_transition(State::Freeze, 0u, &anims.HurtLowerTumble, 0u, &anims.TumbleLoop);
                else if (hurt.region == BlobRegion::Upper)
                    state_transition(State::Freeze, 0u, &anims.HurtUpperTumble, 0u, &anims.TumbleLoop);
            }
            else
            {
                mFrozenState = State::AirStun;

                if (mHitStunTime >= MIN_HITSTUN_HEAVY)
                    state_transition(State::Freeze, 0u, &anims.HurtAirHeavy, 0u, &anims.FallingLoop);
                else
                    state_transition(State::Freeze, 0u, &anims.HurtAirLight, 0u, &anims.FallingLoop);
            }
        }
        else
        {
            mFrozenState = State::Stun;

            if (mHitStunTime >= MIN_HITSTUN_HEAVY)
            {
                if (hurt.region == BlobRegion::Middle)
                    state_transition(State::Freeze, 0u, &anims.HurtMiddleHeavy, 0u, &anims.NeutralLoop);
                else if (hurt.region == BlobRegion::Lower)
                    state_transition(State::Freeze, 0u, &anims.HurtLowerHeavy, 0u, &anims.NeutralLoop);
                else if (hurt.region == BlobRegion::Upper)
                    state_transition(State::Freeze, 0u, &anims.HurtUpperHeavy, 0u, &anims.NeutralLoop);
            }
            else
            {
                if (hurt.region == BlobRegion::Middle)
                    state_transition(State::Freeze, 0u, &anims.HurtMiddleLight, 0u, &anims.NeutralLoop);
                else if (hurt.region == BlobRegion::Lower)
                    state_transition(State::Freeze, 0u, &anims.HurtLowerLight, 0u, &anims.NeutralLoop);
                else if (hurt.region == BlobRegion::Upper)
                    state_transition(State::Freeze, 0u, &anims.HurtUpperLight, 0u, &anims.NeutralLoop);
            }
        }
    }


    //--------------------------------------------------------//

    mFreezeTime = freezeTime;
    mFrozenProgress = 0u;

    hit.action->set_flag(ActionFlag::HitCollide, true);

    // happens when fighters hit each other at the same time, or if a fighter hits multiple others
    if (hit.action->fighter.status.state != State::Freeze)
    {
        hit.action->fighter.mFreezeTime = freezeTime;
        hit.action->fighter.mFrozenProgress = hit.action->fighter.mStateProgress;
        hit.action->fighter.mFrozenState = hit.action->fighter.status.state;

        hit.action->fighter.state_transition(State::Freeze, 0u, nullptr, 0u, nullptr);
    }

    // feels wrong to call a wren_ method here, but Action::mSounds is private
    hit.action->wren_play_sound(hit.sound);
}

//============================================================================//

void Fighter::apply_shield_break()
{
    //mHitStunTime = 240u; // 5 seconds
    //mFrozenState =
}

//============================================================================//

void Fighter::pass_boundary()
{
    status = Status();
    state_transition(State::Neutral, 0u, &mAnimations.NeutralLoop, 0u, nullptr);
}

//============================================================================//

Mat4F Fighter::get_bone_matrix(int8_t bone) const
{
    SQASSERT(bone < int8_t(mArmature.get_bone_count()), "invalid bone");
    if (bone < 0) return mModelMatrix;
    return mModelMatrix * maths::transpose(Mat4F(mBoneMatrices[bone]));
}

bool Fighter::should_render_flinch_models() const
{
    // in smash proper, this is done with a flag that gets set by various actions
    // this is much more limited, but works well enough for now

    const bool stun = status.state == State::Stun || status.state == State::AirStun || status.state == State::TumbleStun;
    const bool freezeStun = mFrozenState == State::Stun || mFrozenState == State::AirStun || mFrozenState == State::TumbleStun;
    const bool tumbleState = status.state == State::TumbleFall || status.state == State::Prone;
    const bool tumbleAction = status.state == State::Action && mActiveAction->type == ActionType::LandTumble;

    return stun || (status.state == State::Freeze && freezeStun) || tumbleState || tumbleAction;
}

//============================================================================//

Action* Fighter::get_action(ActionType type)
{
    if (type == ActionType::None) return nullptr;
    return mActions[int8_t(type)].get();
}
