#include "game/Fighter.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>

#include "game/ActionBuilder.hpp"
#include "game/private/PrivateFighter.hpp"

namespace maths = sq::maths;

using sq::literals::operator""_fmt_;

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

const sq::Armature& Fighter::get_armature() const { return impl->armature; };

//============================================================================//

void Fighter::set_controller(Controller* controller) { impl->controller = controller; }

Controller* Fighter::get_controller() { return impl->controller; }

//============================================================================//

void Fighter::base_tick_fighter() { impl->base_tick_fighter(); }

void Fighter::base_tick_animation() { impl->base_tick_animation(); }

//============================================================================//

void Fighter::apply_hit_basic(const HitBlob& hit)
{
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
    const float rotY = current.state == State::EditorPreview ? 0.5f : 0.25f * float(current.facing);
    const QuatF rotation = QuatF(0.f, rotY, 0.f);
    return maths::transform(Vec3F(position, 0.f), rotation, Vec3F(1.f));
}

void Fighter::interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const
{
    SQASSERT (len == impl->armature.get_bone_names().size(), "");
    auto blendPose = impl->armature.blend_poses(impl->previous.pose, impl->current.pose, blend);
    impl->armature.compute_ubo_data(blendPose, out, uint(len));
}

//============================================================================//

Action* Fighter::get_action(ActionType type)
{
    if (type == ActionType::None) return nullptr;
    return actions[int8_t(type)].get();
}

//Animation* Fighter::get_animation(AnimationType type)
//{
//    if (type == AnimationType::None) return nullptr;
//    return &animations[int8_t(type)];
//}

//============================================================================//

void Fighter::debug_show_fighter_widget()
{
    const String label = "Fighter %d - %s"_fmt_(index, type);
    if (ImGui::CollapsingHeader(label.c_str()) == false) return;

    //--------------------------------------------------------//

    {
        const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

        ImPlus::Text("Position: %s"_fmt_(impl->current.position));
        ImPlus::Text("Velocity: %s"_fmt_(mVelocity));
        ImPlus::Text("Damage: %0.f%%"_fmt_(status.damage));

        ImPlus::Text("state: %s"_fmt_(current.state));

        const String actionName = current.action == nullptr ? "None" :
            "%s (%d)"_fmt_(current.action->get_type(), current.action->mCurrentFrame);

        ImPlus::Text("action: %s"_fmt_(actionName));
    }

    if (ImGui::CollapsingHeader("Edit Stats"))
    {
        const ImPlus::ScopeFont font = ImPlus::FONT_MONO;
        const ImPlus::ScopeItemWidth width = 160.f;

        ImPlus::InputValue("walk_speed",     stats.walk_speed,     0.05f, "%.2f");
        ImPlus::InputValue("dash_speed",     stats.dash_speed,     0.05f, "%.2f");
        ImPlus::InputValue("air_speed",      stats.air_speed,      0.05f, "%.2f");
        ImPlus::InputValue("traction",       stats.traction,       0.05f, "%.2f");
        ImPlus::InputValue("air_mobility",   stats.air_mobility,   0.05f, "%.2f");
        ImPlus::InputValue("air_friction",   stats.air_friction,   0.05f, "%.2f");
        ImPlus::InputValue("hop_height",     stats.hop_height,     0.05f, "%.2f");
        ImPlus::InputValue("jump_height",    stats.jump_height,    0.05f, "%.2f");
        ImPlus::InputValue("air_hop_height", stats.air_hop_height, 0.05f, "%.2f");
        ImPlus::InputValue("gravity",        stats.gravity,        0.05f, "%.2f");
        ImPlus::InputValue("fall_speed",     stats.fall_speed,     0.05f, "%.2f");
        ImPlus::InputValue("evade_distance", stats.evade_distance, 0.05f, "%.2f");

        ImGui::Separator();

        ImPlus::InputValue("dodge_finish",         stats.dodge_finish,         1);
        ImPlus::InputValue("dodge_safe_start",     stats.dodge_safe_start,     1);
        ImPlus::InputValue("dodge_safe_end",       stats.dodge_safe_end,       1);
        ImPlus::InputValue("evade_finish",         stats.evade_finish,         1);
        ImPlus::InputValue("evade_safe_start",     stats.evade_safe_start,     1);
        ImPlus::InputValue("evade_safe_end",       stats.evade_safe_end,       1);
        ImPlus::InputValue("air_dodge_finish",     stats.air_dodge_finish,     1);
        ImPlus::InputValue("air_dodge_safe_start", stats.air_dodge_safe_start, 1);
        ImPlus::InputValue("air_dodge_safe_end",   stats.air_dodge_safe_end,   1);

        ImGui::Separator();

        ImPlus::InputValue("anim_walk_stride", stats.anim_walk_stride, 0.1f);
        ImPlus::InputValue("anim_dash_stride", stats.anim_dash_stride, 0.1f);

        ImGui::Separator();
    }

    //--------------------------------------------------------//

    {
        const ImPlus::ScopeFont font = ImPlus::FONT_REGULAR;

        if (ImGui::Button("RESET"))
        {
            impl->current.position = { 0.f, 1.f };
        }
        ImPlus::HoverTooltip("reset the fighter's position");

        ImGui::SameLine();

        if (ImGui::Button("BOUNCE"))
        {
            mVelocity.y = +10.f;
        }
        ImPlus::HoverTooltip("make the fighter bounce");
    }
}

//============================================================================//

void Fighter::debug_reload_actions()
{
    for (int8_t i = 0; i < sq::enum_count_v<ActionType>; ++i)
        mFightWorld.get_action_builder().load_from_json(*actions[i]);
}

//============================================================================//

Vec2F& Fighter::edit_position()
{
    return impl->current.position;
}

Vec2F& Fighter::edit_velocity()
{
    return impl->current.position;
}
