#include "game/Fighter.hpp"

#include "game/Action.hpp"
#include "game/FightWorld.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

namespace maths = sq::maths;
namespace algo = sq::algo;

using namespace sts;

//============================================================================//

Fighter::Fighter(uint8_t index, FightWorld& world, FighterEnum type)
    : index(index), type(type), world(world), mHurtBlobs(world.get_hurt_blob_allocator())
{
    const String path = sq::build_path("assets/fighters", sq::enum_to_string(type));

    initialise_armature(path);
    initialise_hurt_blobs(path);
    initialise_stats(path);
    initialise_actions(path);
}

Fighter::~Fighter() = default;

//============================================================================//

void Fighter::initialise_armature(const String& path)
{
    mArmature.load_bones(path + "/Bones.txt", true);
    mArmature.load_rest_pose(path + "/RestPose.txt");

    current.pose = previous.pose = mArmature.get_rest_pose();

    mBoneMatrices.resize(mArmature.get_bone_count());

    const auto load_anim = [&](const char* name, AnimMode mode) -> Animation
    {
        const bool looping = mode == AnimMode::Looping || mode == AnimMode::WalkCycle || mode == AnimMode::DashCycle;

        String filePath = sq::build_path(path, "anims", name) + ".sqa";
        if (sq::check_file_exists(filePath) == false)
        {
            filePath = sq::build_path(path, "anims", name) + ".txt";
            if (sq::check_file_exists(filePath) == false)
            {
                sq::log_warning("missing animation '{}'", filePath);
                return { mArmature.make_null_animation(1u, looping), mode, name };
            }
        }
        Animation result = { mArmature.make_animation(filePath), mode, name };
        if (!result.anim.looping() && looping) sq::log_warning("animation '{}' should loop", filePath);
        if (result.anim.looping() && !looping) sq::log_warning("animation '{}' should not loop", filePath);
        return result;
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
    anims.Turn = load_anim("Turn", AnimMode::Standard);
    anims.TurnBrake = load_anim("TurnBrake", AnimMode::Standard);
    anims.TurnDash = load_anim("TurnDash", AnimMode::Standard);

    anims.JumpBack = load_anim("JumpBack", AnimMode::Standard);
    anims.JumpForward = load_anim("JumpForward", AnimMode::Standard);
    anims.AirHopBack = load_anim("AirHopBack", AnimMode::Standard);
    anims.AirHopForward = load_anim("AirHopForward", AnimMode::Standard);

    anims.LedgeCatch = load_anim("LedgeCatch", AnimMode::Standard);
    anims.LedgeLoop = load_anim("LedgeLoop", AnimMode::Looping);
    anims.LedgeClimb = load_anim("LedgeClimb", AnimMode::ApplyMotion);
    anims.LedgeJump = load_anim("LedgeJump", AnimMode::Standard);

    anims.NeutralFirst = load_anim("NeutralFirst", AnimMode::Standard);

    anims.DashAttack = load_anim("DashAttack", AnimMode::ApplyMotion);

    anims.TiltDown = load_anim("TiltDown", AnimMode::Standard);
    anims.TiltForward = load_anim("TiltForward", AnimMode::ApplyMotion);
    anims.TiltUp = load_anim("TiltUp", AnimMode::Standard);

    anims.EvadeBack = load_anim("EvadeBack", AnimMode::ApplyMotion);
    anims.EvadeForward = load_anim("EvadeForward", AnimMode::ApplyMotion);
    anims.Dodge = load_anim("Dodge", AnimMode::Standard);

    anims.ProneAttack = load_anim("ProneAttack", AnimMode::Standard);
    anims.ProneBack = load_anim("ProneBack", AnimMode::ApplyMotion);
    anims.ProneForward = load_anim("ProneForward", AnimMode::ApplyMotion);
    anims.ProneStand = load_anim("ProneStand", AnimMode::Standard);

    anims.SmashDownStart = load_anim("SmashDownStart", AnimMode::Standard);
    anims.SmashForwardStart = load_anim("SmashForwardStart", AnimMode::ApplyMotion);
    anims.SmashUpStart = load_anim("SmashUpStart", AnimMode::Standard);

    anims.SmashDownCharge = load_anim("SmashDownCharge", AnimMode::Looping);
    anims.SmashForwardCharge = load_anim("SmashForwardCharge", AnimMode::Looping);
    anims.SmashUpCharge = load_anim("SmashUpCharge", AnimMode::Looping);

    anims.SmashDownAttack = load_anim("SmashDownAttack", AnimMode::Standard);
    anims.SmashForwardAttack = load_anim("SmashForwardAttack", AnimMode::ApplyMotion);
    anims.SmashUpAttack = load_anim("SmashUpAttack", AnimMode::Standard);

    anims.AirBack = load_anim("AirBack", AnimMode::Standard);
    anims.AirDown = load_anim("AirDown", AnimMode::Standard);
    anims.AirForward = load_anim("AirForward", AnimMode::Standard);
    anims.AirNeutral = load_anim("AirNeutral", AnimMode::Standard);
    anims.AirUp = load_anim("AirUp", AnimMode::Standard);
    anims.AirDodge = load_anim("AirDodge", AnimMode::Standard);

    anims.LandLight = load_anim("LandLight", AnimMode::Standard);
    anims.LandHeavy = load_anim("LandHeavy", AnimMode::Standard);
    //anims.LandAirBack = load_anim("LandAirBack", AnimMode::Standard);
    //anims.LandAirDown = load_anim("LandAirDown", AnimMode::Standard);
    //anims.LandAirForward = load_anim("LandAirForward", AnimMode::Standard);
    //anims.LandAirNeutral = load_anim("LandAirNeutral", AnimMode::Standard);
    //anims.LandAirUp = load_anim("LandAirUp", AnimMode::Standard);
    anims.LandTumble = load_anim("LandTumble", AnimMode::Standard);

    anims.HurtLowerLight = load_anim("HurtLowerLight", AnimMode::Standard);
    //anims.HurtLowerMedium = load_anim("HurtLowerMedium", AnimMode::Standard);
    anims.HurtLowerHeavy = load_anim("HurtLowerHeavy", AnimMode::Standard);
    anims.HurtLowerTumble = load_anim("HurtLowerTumble", AnimMode::Standard);

    anims.HurtMiddleLight = load_anim("HurtMiddleLight", AnimMode::Standard);
    //anims.HurtMiddleMedium = load_anim("HurtMiddleMedium", AnimMode::Standard);
    anims.HurtMiddleHeavy = load_anim("HurtMiddleHeavy", AnimMode::Standard);
    anims.HurtMiddleTumble = load_anim("HurtMiddleTumble", AnimMode::Standard);

    anims.HurtUpperLight = load_anim("HurtUpperLight", AnimMode::Standard);
    //anims.HurtUpperMedium = load_anim("HurtUpperMedium", AnimMode::Standard);
    anims.HurtUpperHeavy = load_anim("HurtUpperHeavy", AnimMode::Standard);
    anims.HurtUpperTumble = load_anim("HurtUpperTumble", AnimMode::Standard);

    //anims.LaunchLoop = load_anim("LaunchLoop", AnimMode::Looping);
    //anims.LaunchFinish = load_anim("LaunchFinish", AnimMode::Standard);

    const auto ensure_anim_at_least = [](Animation& anim, uint time, const char* timeName)
    {
        if (anim.anim.totalTime > time) return; // anim is longer than time

        if (anim.anim.totalTime != 1u) // fallback animation, don't print another warning
            sq::log_warning("anim '{}' shorter than '{}'", anim.key, timeName);

        anim.anim.totalTime = anim.anim.times.front() = time + 1u;
    };

    ensure_anim_at_least(anims.DashStart, stats.dash_start_time, "dash_start_time");
    ensure_anim_at_least(anims.Brake, stats.dash_brake_time, "dash_brake_time");
    ensure_anim_at_least(anims.TurnDash, stats.dash_turn_time, "dash_turn_time");

    ensure_anim_at_least(anims.HurtLowerHeavy, MIN_HITSTUN_HEAVY, "MIN_HITSTUN_HEAVY");
    ensure_anim_at_least(anims.HurtMiddleHeavy, MIN_HITSTUN_HEAVY, "MIN_HITSTUN_HEAVY");
    ensure_anim_at_least(anims.HurtUpperHeavy, MIN_HITSTUN_HEAVY, "MIN_HITSTUN_HEAVY");

    ensure_anim_at_least(anims.HurtLowerTumble, MIN_HITSTUN_TUMBLE, "MIN_HITSTUN_TUMBLE");
    ensure_anim_at_least(anims.HurtMiddleTumble, MIN_HITSTUN_TUMBLE, "MIN_HITSTUN_TUMBLE");
    ensure_anim_at_least(anims.HurtUpperTumble, MIN_HITSTUN_TUMBLE, "MIN_HITSTUN_TUMBLE");
}

//============================================================================//

void Fighter::initialise_hurt_blobs(const String& path)
{
    const JsonValue root = sq::parse_json_from_file(path + "/HurtBlobs.json");
    for (const auto& item : root.items())
    {
        HurtBlob& blob = mHurtBlobs[item.key()];
        blob.fighter = this;

        try { blob.from_json(item.value()); }
        catch (const std::exception& e) {
            sq::log_warning("problem loading hurt blob '{}': {}", item.key(), e.what());
        }

        world.enable_hurt_blob(&blob);
    }
}

//============================================================================//

void Fighter::initialise_stats(const String& path)
{
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

    stats.anim_walk_stride = json.at("anim_walk_stride");
    stats.anim_dash_stride = json.at("anim_dash_stride");

    // todo: move this to the per-fighter lua init script

    mLocalDiamond = LocalDiamond(json.at("diamond_halfWidth"), json.at("diamond_offsetTop"), json.at("diamond_offsetCross"));
}

//============================================================================//

void Fighter::initialise_actions(const String& path)
{
    for (int8_t i = 0; i < sq::enum_count_v<ActionType>; ++i)
    {
        const auto actionType = ActionType(i);
        const auto actionPath = sq::build_string(path, "/actions/", sq::to_c_string(actionType));

        mActions[i] = std::make_unique<Action>(world, *this, actionType, actionPath);

        mActions[i]->load_from_json();
        mActions[i]->load_lua_from_file();
    }
}

//============================================================================//

bool Fighter::consume_command(Fighter::Command cmd)
{
    for (uint i = 0u; i < 8u; ++i)
    {
        std::vector<Command>& vec = mCommands[7u-i];
        auto iter = std::find(vec.rbegin(), vec.rend(), cmd);
        if (iter != vec.rend())
        {
            vec.erase(std::prev(iter.base()));
            return true;
        }
    }

    return false;
}

bool Fighter::consume_command_oldest(std::initializer_list<Command> cmds)
{
    for (uint i = 0u; i < 8u; ++i)
    {
        std::vector<Command>& vec = mCommands[7u-i];
        for (auto iter = vec.rbegin(); iter != vec.rend(); ++iter)
        {
            auto findIter = algo::find(cmds, *iter);
            if (findIter != cmds.end())
            {
                vec.erase(std::prev(iter.base()));
                return true;
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

bool Fighter::consume_command_oldest_facing(std::initializer_list<Command> leftCmds, std::initializer_list<Command> rightCmds)
{
    if (status.facing == -1) return consume_command_oldest(leftCmds);
    if (status.facing == +1) return consume_command_oldest(rightCmds);
    return false; // make compiler happy
}

//============================================================================//

void Fighter::apply_hit_basic(const HitBlob& hit, const HurtBlob& hurt)
{
    SQASSERT(hit.fighter != this, "invalid hitblob");
    SQASSERT(hurt.fighter == this, "invalid hurtblob");

    status.damage += hit.damage;

    // if we were in the middle of an action, cancel it
    if (mActiveAction != nullptr) mActiveAction->do_cancel();

    //--------------------------------------------------------//

    // knockback formula: https://www.ssbwiki.com/Knockback#Melee_onward
    // freezetime formula: https://www.ssbwiki.com/Hitlag#Formula
    // knockback and weight values are the same arbitary units as in smash bros
    // hitstun should always end before launch speed completely decays

    const float angle = maths::radians(hit.knockAngle / 360.f);
    const Vec2F knockDir = { std::cos(angle) * float(hit.fighter->status.facing), std::sin(angle) };

    const float damageFactor = status.damage / 10.f + (status.damage * hit.damage) / 20.f;
    const float weightFactor = 200.f / (stats.weight + 100.f);

    const float knockback = hit.knockBase + (damageFactor * weightFactor * 1.4f + 18.f) * hit.knockScale * 0.01f;

    const uint freezeTime = uint((hit.damage * 0.4f + 4.f) * hit.freezeFactor);

    const float launchSpeed = knockback * 0.003f;
    const uint hitStunTime = uint(knockback * 0.4f);

    sq::log_debug_multiline("fighter {} hit by fighter {}:"
                            "\naction:      {}" "\nknockback:   {}" "\nfreezeTime:  {}" "\nhitStun:     {}"
                            "\nknockDir:    {}" "\nlaunchSpeed: {}" "\ndecayFrames: {}" "\nhurtRegion:  {}",
                            index, hit.fighter->index,
                            hit.action->type, knockback, freezeTime, hitStunTime,
                            knockDir, launchSpeed, uint(launchSpeed / KNOCKBACK_DECAY), hurt.region);

    SQASSERT(hitStunTime < uint(launchSpeed / KNOCKBACK_DECAY), "check ya formulas");

    //--------------------------------------------------------//

    mLaunchSpeed = launchSpeed;
    mHitStunTime = hitStunTime;

    mFreezeTime = freezeTime;
    mFrozenProgress = 0u;

    status.velocity = maths::normalize(knockDir) * launchSpeed;
    status.facing = -hit.fighter->status.facing;

    //--------------------------------------------------------//

    // todo: in the air, should be always using those animations regardless of region

    const auto& anims = mAnimations;

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
    else if (mHitStunTime >= MIN_HITSTUN_HEAVY)
    {
        mFrozenState = State::HitStun;

        if (hurt.region == BlobRegion::Middle)
            state_transition(State::Freeze, 0u, &anims.HurtMiddleHeavy, 4u, &anims.FallingLoop);
        else if (hurt.region == BlobRegion::Lower)
            state_transition(State::Freeze, 0u, &anims.HurtLowerHeavy, 4u, &anims.FallingLoop);
        else if (hurt.region == BlobRegion::Upper)
            state_transition(State::Freeze, 0u, &anims.HurtUpperHeavy, 4u, &anims.FallingLoop);
    }
    else
    {
        mFrozenState = State::HitStun;

        if (hurt.region == BlobRegion::Middle)
            state_transition(State::Freeze, 0u, &anims.HurtMiddleLight, 4u, &anims.FallingLoop);
        else if (hurt.region == BlobRegion::Lower)
            state_transition(State::Freeze, 0u, &anims.HurtLowerLight, 4u, &anims.FallingLoop);
        else if (hurt.region == BlobRegion::Upper)
            state_transition(State::Freeze, 0u, &anims.HurtUpperLight, 4u, &anims.FallingLoop);
    }

    //--------------------------------------------------------//

    hit.fighter->mFreezeTime = freezeTime;
    hit.fighter->mFrozenProgress = hit.fighter->mStateProgress;
    hit.fighter->mFrozenState = hit.fighter->status.state;

    hit.fighter->state_transition(State::Freeze, 0u, nullptr, 0u, nullptr);
}

//============================================================================//

void Fighter::pass_boundary()
{
    current.position = Vec2F();
    status.velocity = Vec2F();

    state_transition(State::Neutral, 0u, &mAnimations.NeutralLoop, 0u, nullptr);
}

//============================================================================//

Mat4F Fighter::get_bone_matrix(int8_t bone) const
{
    SQASSERT(bone < int8_t(mArmature.get_bone_count()), "invalid bone");
    if (bone < 0) return mModelMatrix;
    return mModelMatrix * maths::transpose(Mat4F(mBoneMatrices[bone]));
}

Mat4F Fighter::interpolate_model_matrix(float blend) const
{
    const Vec2F position = maths::mix(previous.position, current.position, blend);
    const QuatF rotation = maths::slerp(previous.rotation, current.rotation, blend);
    return maths::transform(Vec3F(position, 0.f), rotation, Vec3F(1.f));
}

void Fighter::interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const
{
    SQASSERT(len == mArmature.get_bone_count(), "buffer size does not match");
    const auto blendPose = mArmature.blend_poses(previous.pose, current.pose, blend);
    mArmature.compute_ubo_data(blendPose, out, uint(len));
}

//============================================================================//

Action* Fighter::get_action(ActionType type)
{
    if (type == ActionType::None) return nullptr;
    return mActions[int8_t(type)].get();
}

//============================================================================//

void Fighter::debug_reload_actions()
{
    for (int8_t i = 0; i < sq::enum_count_v<ActionType>; ++i)
    {
        mActions[i]->load_from_json();
        mActions[i]->load_lua_from_file();
    }
}

//============================================================================//

std::vector<Mat4F> Fighter::debug_get_skeleton_mats() const
{
    return mArmature.compute_skeleton_matrices(current.pose);
}
