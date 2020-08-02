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
    anims.LandLight = load_anim("LandLight", AnimMode::Standard);
    anims.LandHeavy = load_anim("LandHeavy", AnimMode::Standard);
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

    anims.EvadeBack = load_anim("EvadeBack", AnimMode::ApplyMotion);
    anims.EvadeForward = load_anim("EvadeForward", AnimMode::ApplyMotion);
    anims.Dodge = load_anim("Dodge", AnimMode::Standard);
    anims.AirDodge = load_anim("AirDodge", AnimMode::Standard);

    anims.NeutralFirst = load_anim("NeutralFirst", AnimMode::Standard);

    anims.TiltDown = load_anim("TiltDown", AnimMode::Standard);
    anims.TiltForward = load_anim("TiltForward", AnimMode::ApplyMotion);
    anims.TiltUp = load_anim("TiltUp", AnimMode::Standard);

    anims.AirBack = load_anim("AirBack", AnimMode::Standard);
    anims.AirDown = load_anim("AirDown", AnimMode::Standard);
    anims.AirForward = load_anim("AirForward", AnimMode::Standard);
    anims.AirNeutral = load_anim("AirNeutral", AnimMode::Standard);
    anims.AirUp = load_anim("AirUp", AnimMode::Standard);

    anims.DashAttack = load_anim("DashAttack", AnimMode::ApplyMotion);

    anims.SmashDownStart = load_anim("SmashDownStart", AnimMode::Standard);
    anims.SmashForwardStart = load_anim("SmashForwardStart", AnimMode::ApplyMotion);
    anims.SmashUpStart = load_anim("SmashUpStart", AnimMode::Standard);

    anims.SmashDownCharge = load_anim("SmashDownCharge", AnimMode::Looping);
    anims.SmashForwardCharge = load_anim("SmashForwardCharge", AnimMode::Looping);
    anims.SmashUpCharge = load_anim("SmashUpCharge", AnimMode::Looping);

    anims.SmashDownAttack = load_anim("SmashDownAttack", AnimMode::Standard);
    anims.SmashForwardAttack = load_anim("SmashForwardAttack", AnimMode::ApplyMotion);
    anims.SmashUpAttack = load_anim("SmashUpAttack", AnimMode::Standard);

    // some anims need to be longer than a certain length for logic to work correctly
    const auto ensure_anim_time = [](Animation& anim, uint time, const char* animName, const char* timeName)
    {
        if (anim.anim.totalTime > time) return; // anim is longer than time

        if (anim.anim.totalTime != 1u) // fallback animation, don't print another warning
            sq::log_warning("anim '{}' shorter than '{}'", animName, timeName);

        anim.anim.totalTime = anim.anim.times.front() = time + 1u;
    };

    ensure_anim_time(anims.DashStart, stats.dash_start_time, "DashStart", "dash_start_time");
    ensure_anim_time(anims.Brake, stats.dash_brake_time, "Brake", "dash_brake_time");
    ensure_anim_time(anims.TurnDash, stats.dash_turn_time, "TurnDash", "dash_turn_time");
    ensure_anim_time(anims.LedgeClimb, stats.ledge_climb_time, "LedgeClimb", "ledge_climb_time");
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
    stats.ledge_climb_time = json.at("ledge_climb_time");

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

void Fighter::apply_hit_basic(const HitBlob& hit)
{
    auto func = &Fighter::apply_hit_basic;
    (this->*func)(hit);

    const float angle = maths::radians(hit.knockAngle * float(hit.fighter->status.facing));
    const Vec2F knockDir = { std::sin(angle), std::cos(angle) };

    status.damage += hit.damage;

    const float damageFactor = status.damage / 10.f + (status.damage * hit.damage) / 20.f;
    const float weightFactor = (2.f / (stats.weight + 1.f)); // * 1.4f;

    const float knockback = damageFactor * weightFactor * hit.knockScale / 10.f + hit.knockBase;

    status.velocity = maths::normalize(knockDir) * knockback;

    status.state = State::Knocked;
}

void Fighter::pass_boundary()
{
    current.position = Vec2F();
    status.velocity = Vec2F();
}

//============================================================================//

Mat4F Fighter::interpolate_model_matrix(float blend) const
{
    const Vec2F position = maths::mix(previous.position, current.position, blend);
    const QuatF rotation = maths::slerp(previous.rotation, current.rotation, blend);

    return maths::transform(Vec3F(position, 0.f), rotation, Vec3F(1.f));
}

void Fighter::interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const
{
    SQASSERT(len == mArmature.get_bone_names().size(), "");
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
