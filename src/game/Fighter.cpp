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

Fighter::Fighter(uint8_t index, FightWorld& world, StringView name)
    : index(index), mFightWorld(world), mName(name)
{
    impl = std::make_unique<PrivateFighter>(*this);

    const String path = sq::build_path("assets/fighters", name);

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
    const QuatF rotation = QuatF(0.f, 0.25f * float(current.facing), 0.f);
    return maths::transform(Vec3F(position, 0.f), rotation, Vec3F(1.f));
}

void Fighter::interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const
{
    auto blendPose = impl->armature.blend_poses(impl->previous.pose, impl->current.pose, blend);
    impl->armature.compute_ubo_data(blendPose, out, uint(len));
}

//============================================================================//

Action* Fighter::get_action(ActionType type)
{
    SWITCH (type) {
    CASE (Neutral_First)    return actions.neutral_first.get();
    CASE (Tilt_Down)        return actions.tilt_down.get();
    CASE (Tilt_Forward)     return actions.tilt_forward.get();
    CASE (Tilt_Up)          return actions.tilt_up.get();
    CASE (Air_Back)         return actions.air_back.get();
    CASE (Air_Down)         return actions.air_down.get();
    CASE (Air_Forward)      return actions.air_forward.get();
    CASE (Air_Neutral)      return actions.air_neutral.get();
    CASE (Air_Up)           return actions.air_up.get();
    CASE (Dash_Attack)      return actions.dash_attack.get();
    CASE (Smash_Down)       return actions.smash_down.get();
    CASE (Smash_Forward)    return actions.smash_forward.get();
    CASE (Smash_Up)         return actions.smash_up.get();
    CASE (Special_Down)     return actions.special_down.get();
    CASE (Special_Forward)  return actions.special_forward.get();
    CASE (Special_Neutral)  return actions.special_neutral.get();
    CASE (Special_Up)       return actions.special_up.get();
    CASE (None)             return nullptr;
    } SWITCH_END;
}

//============================================================================//

void Fighter::debug_show_fighter_widget()
{
    const String label = "Fighter %d - %s"_fmt_(index, mName);
    if (ImGui::CollapsingHeader(label.c_str()) == false) return;

    //--------------------------------------------------------//

    {
        const ImGui::ScopeFont font = ImGui::FONT_MONO;

        ImGui::Text("Position: %s"_fmt_(impl->current.position));
        ImGui::Text("Velocity: %s"_fmt_(mVelocity));
        ImGui::Text("Damage: %0.f%%"_fmt_(status.damage));

        ImGui::Text("state: %s"_fmt_(current.state));

        const String actionName = current.action == nullptr ? "None" :
            "%s (%d)"_fmt_(current.action->get_type(), current.action->mCurrentFrame);

        ImGui::Text("action: %s"_fmt_(actionName));
    }

    if (ImGui::CollapsingHeader("Edit Stats"))
    {
        const ImGui::ScopeFont font = ImGui::FONT_MONO;
        const ImGui::ScopeItemWidth width = 160.f;

        ImGui::InputValue("walk_speed",     stats.walk_speed,     0.05f, '2');
        ImGui::InputValue("dash_speed",     stats.dash_speed,     0.05f, '2');
        ImGui::InputValue("air_speed",      stats.air_speed,      0.05f, '2');
        ImGui::InputValue("traction",       stats.traction,       0.05f, '2');
        ImGui::InputValue("air_mobility",   stats.air_mobility,   0.05f, '2');
        ImGui::InputValue("air_friction",   stats.air_friction,   0.05f, '2');
        ImGui::InputValue("hop_height",     stats.hop_height,     0.05f, '2');
        ImGui::InputValue("jump_height",    stats.jump_height,    0.05f, '2');
        ImGui::InputValue("air_hop_height", stats.air_hop_height, 0.05f, '2');
        ImGui::InputValue("gravity",        stats.gravity,        0.05f, '2');
        ImGui::InputValue("fall_speed",     stats.fall_speed,     0.05f, '2');
        ImGui::InputValue("evade_distance", stats.evade_distance, 0.05f, '2');

        ImGui::Separator();

        ImGui::InputValue("dodge_finish",         stats.dodge_finish,         1);
        ImGui::InputValue("dodge_safe_start",     stats.dodge_safe_start,     1);
        ImGui::InputValue("dodge_safe_end",       stats.dodge_safe_end,       1);
        ImGui::InputValue("evade_finish",         stats.evade_finish,         1);
        ImGui::InputValue("evade_safe_start",     stats.evade_safe_start,     1);
        ImGui::InputValue("evade_safe_end",       stats.evade_safe_end,       1);
        ImGui::InputValue("air_dodge_finish",     stats.air_dodge_finish,     1);
        ImGui::InputValue("air_dodge_safe_start", stats.air_dodge_safe_start, 1);
        ImGui::InputValue("air_dodge_safe_end",   stats.air_dodge_safe_end,   1);

        ImGui::Separator();
    }

    //--------------------------------------------------------//

    {
        const ImGui::ScopeFont font = ImGui::FONT_REGULAR;

        if (ImGui::Button("RESET"))
        {
            impl->current.position = { 0.f, 1.f };
        }
        ImGui::HoverTooltip("reset the fighter's position");

        ImGui::SameLine();

        if (ImGui::Button("BOUNCE"))
        {
            mVelocity.y = +10.f;
        }
        ImGui::HoverTooltip("make the fighter bounce");
    }
}

//============================================================================//

void Fighter::debug_reload_actions()
{
    ActionBuilder& actionBuilder = mFightWorld.get_action_builder();

    actionBuilder.load_from_json(*actions.neutral_first);

    actionBuilder.load_from_json(*actions.tilt_down);
    actionBuilder.load_from_json(*actions.tilt_forward);
    actionBuilder.load_from_json(*actions.tilt_up);

    actionBuilder.load_from_json(*actions.air_back);
    actionBuilder.load_from_json(*actions.air_down);
    actionBuilder.load_from_json(*actions.air_forward);
    actionBuilder.load_from_json(*actions.air_neutral);
    actionBuilder.load_from_json(*actions.air_up);

    actionBuilder.load_from_json(*actions.dash_attack);

    actionBuilder.load_from_json(*actions.smash_down);
    actionBuilder.load_from_json(*actions.smash_forward);
    actionBuilder.load_from_json(*actions.smash_up);

    actionBuilder.load_from_json(*actions.special_down);
    actionBuilder.load_from_json(*actions.special_forward);
    actionBuilder.load_from_json(*actions.special_neutral);
    actionBuilder.load_from_json(*actions.special_up);
}
