#include "game/Fighter.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Algorithms.hpp>
#include <sqee/misc/Files.hpp>

#include "game/private/PrivateFighter.hpp"

namespace maths = sq::maths;
namespace algo = sq::algo;

using namespace sts;

//============================================================================//

Fighter::Fighter(uint8_t index, FightWorld& world, FighterEnum type)
    : index(index), type(type), hurtBlobs(world.get_hurt_blob_allocator()), mFightWorld(world)
{
    impl = std::make_unique<PrivateFighter>(*this);

    const String path = sq::build_path("assets/fighters", sq::to_c_string(type));

    impl->initialise_armature(path);
    impl->initialise_hurt_blobs(path);
    impl->initialise_stats(path);
    impl->initialise_actions(path);
}

Fighter::~Fighter() = default;

//============================================================================//

bool Fighter::consume_command(Fighter::Command cmd)
{
    for (uint i = 0u; i < 8u; ++i)
    {
        Vector<Command>& vec = mCommands[7u-i];
        auto iter = std::find(vec.rbegin(), vec.rend(), cmd);
        if (iter != vec.rend())
        {
            vec.erase(std::prev(iter.base()));
            return true;
        }
    }

    return false;
}

bool Fighter::consume_command_oldest(InitList<Command> cmds)
{
    for (uint i = 0u; i < 8u; ++i)
    {
        Vector<Command>& vec = mCommands[7u-i];
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
    if (current.facing == -1) return consume_command(leftCmd);
    if (current.facing == +1) return consume_command(rightCmd);
    return false; // make compiler happy
}

bool Fighter::consume_command_oldest_facing(InitList<Command> leftCmds, InitList<Command> rightCmds)
{
    if (current.facing == -1) return consume_command_oldest(leftCmds);
    if (current.facing == +1) return consume_command_oldest(rightCmds);
    return false; // make compiler happy
}

//============================================================================//

const sq::Armature& Fighter::get_armature() const { return impl->armature; };

void Fighter::set_controller(Controller* controller) { impl->controller = controller; }

Controller* Fighter::get_controller() { return impl->controller; }

void Fighter::base_tick_fighter() { impl->base_tick_fighter(); }

//============================================================================//

void Fighter::apply_hit_basic(const HitBlob& hit)
{
    auto func = &Fighter::apply_hit_basic;
    (this->*func)(hit);

    const float angle = maths::radians(hit.knockAngle * float(hit.fighter->current.facing));
    const Vec2F knockDir = { std::sin(angle), std::cos(angle) };

    status.damage += hit.damage;

    const float damageFactor = status.damage / 10.f + (status.damage * hit.damage) / 20.f;
    const float weightFactor = (2.f / (stats.weight + 1.f)); // * 1.4f;

    const float knockback = damageFactor * weightFactor * hit.knockScale / 10.f + hit.knockBase;

    mVelocity = maths::normalize(knockDir) * knockback;

    current.state = State::Knocked;
}

void Fighter::pass_boundary()
{
    impl->current.position = Vec2F();
    mVelocity = Vec2F();
}

//============================================================================//

Mat4F Fighter::interpolate_model_matrix(float blend) const
{
    const Vec2F position = maths::mix(impl->previous.position, impl->current.position, blend);
    const QuatF rotation = maths::slerp(impl->previous.rotation, impl->current.rotation, blend);

    return maths::transform(Vec3F(position, 0.f), rotation, Vec3F(1.f));
}

void Fighter::interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const
{
    SQASSERT(len == impl->armature.get_bone_names().size(), "");
    auto blendPose = impl->armature.blend_poses(impl->previous.pose, impl->current.pose, blend);
    impl->armature.compute_ubo_data(blendPose, out, uint(len));
}

//============================================================================//

Action* Fighter::get_action(ActionType type)
{
    if (type == ActionType::None) return nullptr;
    return actions[int8_t(type)].get();
}

//============================================================================//

void Fighter::debug_reload_actions()
{
    for (int8_t i = 0; i < sq::enum_count_v<ActionType>; ++i)
    {
        actions[i]->load_from_json();
        actions[i]->load_lua_from_file();
    }
}

//============================================================================//

Vec2F Fighter::get_position() const
{
    return impl->current.position;
}

Vec2F Fighter::get_velocity() const
{
    return mVelocity;
}

//============================================================================//

Vector<Mat4F> Fighter::debug_get_skeleton_mats() const
{
    return impl->armature.compute_skeleton_matrices(impl->current.pose);
}
