#include <sqee/assert.hpp>
#include <sqee/debug/Logging.hpp>

#include <sqee/app/GuiWidgets.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/maths/Functions.hpp>

#include "game/private/PrivateFighter.hpp"

#include "game/ActionBuilder.hpp"
#include "game/Fighter.hpp"

namespace maths = sq::maths;
namespace gui = sq::gui;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

Fighter::Fighter(uint8_t index, FightWorld& world, string_view name)
    : index(index), mFightWorld(world), mName(name)
{
    impl = std::make_unique<PrivateFighter>(*this);

    const string path = sq::build_path("assets/fighters", name);

    impl->initialise_armature(path);
    impl->initialise_hurt_blobs(path);
    impl->initialise_stats(path);
    impl->initialise_actions(path);
}

Fighter::~Fighter() = default;

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

std::vector<Mat34F> Fighter::interpolate_bone_matrices(float blend) const
{
    auto blendPose = impl->armature.blend_poses(impl->previous.pose, impl->current.pose, blend);
    return impl->armature.compute_ubo_data(blendPose);
}

//============================================================================//

void Fighter::debug_show_fighter_widget()
{
    const auto widget = gui::scope_collapse("Fighter %d - %s"_fmt_(index, mName), {}, gui::FONT_BOLD);

    if (widget.want_display() == false) return;

    //--------------------------------------------------------//

    WITH (gui::scope_framed_group())
    {
        const auto font = gui::scope_font(gui::FONT_MONO);

        gui::display_text("Position: %s"_fmt_(impl->current.position));
        gui::display_text("Velocity: %s"_fmt_(mVelocity));
        gui::display_text("Damage: %0.f%%"_fmt_(status.damage));

        gui::display_text("state: %s"_fmt_(enum_to_string(current.state)));
    }

    //--------------------------------------------------------//

    WITH_IF (gui::scope_collapse("Edit Stats", {}, gui::FONT_REGULAR))
    {
        const auto font = gui::scope_font(gui::FONT_MONO);

        gui::input_float("walk_speed",     140.f, stats.walk_speed,     0.05f, 2);
        gui::input_float("dash_speed",     140.f, stats.dash_speed,     0.05f, 2);
        gui::input_float("air_speed",      140.f, stats.air_speed,      0.05f, 2);
        gui::input_float("traction",       140.f, stats.traction,       0.05f, 2);
        gui::input_float("air_mobility",   140.f, stats.air_mobility,   0.05f, 2);
        gui::input_float("air_friction",   140.f, stats.air_friction,   0.05f, 2);
        gui::input_float("hop_height",     140.f, stats.hop_height,     0.05f, 2);
        gui::input_float("jump_height",    140.f, stats.jump_height,    0.05f, 2);
        gui::input_float("air_hop_height", 140.f, stats.air_hop_height, 0.05f, 2);
        gui::input_float("gravity",        140.f, stats.gravity,        0.05f, 2);
        gui::input_float("fall_speed",     140.f, stats.fall_speed,     0.05f, 2);
        gui::input_float("evade_distance", 140.f, stats.evade_distance, 0.05f, 2);

        gui::input_int("dodge_finish",         160.f, stats.dodge_finish,         1);
        gui::input_int("dodge_safe_start",     160.f, stats.dodge_safe_start,     1);
        gui::input_int("dodge_safe_end",       160.f, stats.dodge_safe_end,       1);
        gui::input_int("evade_finish",         160.f, stats.evade_finish,         1);
        gui::input_int("evade_safe_start",     160.f, stats.evade_safe_start,     1);
        gui::input_int("evade_safe_end",       160.f, stats.evade_safe_end,       1);
        gui::input_int("air_dodge_finish",     160.f, stats.air_dodge_finish,     1);
        gui::input_int("air_dodge_safe_start", 160.f, stats.air_dodge_safe_start, 1);
        gui::input_int("air_dodge_safe_end",   160.f, stats.air_dodge_safe_end,   1);
    }

    //--------------------------------------------------------//

    WITH (gui::scope_framed_group())
    {
        const auto font = gui::scope_font(gui::FONT_REGULAR);

        if (gui::button_with_tooltip("RESET", "reset the fighter's position"))
            impl->current.position = { 0.f, 1.f };

        imgui::SameLine();

        if (gui::button_with_tooltip("BOUNCE", "make the fighter bounce"))
            mVelocity.y = +10.f;
    }
}

//============================================================================//

void Fighter::debug_reload_actions()
{
    ActionBuilder::load_from_json(*actions.neutral_first);

    ActionBuilder::load_from_json(*actions.tilt_down);
    ActionBuilder::load_from_json(*actions.tilt_forward);
    ActionBuilder::load_from_json(*actions.tilt_up);

    ActionBuilder::load_from_json(*actions.air_back);
    ActionBuilder::load_from_json(*actions.air_down);
    ActionBuilder::load_from_json(*actions.air_forward);
    ActionBuilder::load_from_json(*actions.air_neutral);
    ActionBuilder::load_from_json(*actions.air_up);

    ActionBuilder::load_from_json(*actions.dash_attack);

    ActionBuilder::load_from_json(*actions.smash_down);
    ActionBuilder::load_from_json(*actions.smash_forward);
    ActionBuilder::load_from_json(*actions.smash_up);

    ActionBuilder::load_from_json(*actions.special_down);
    ActionBuilder::load_from_json(*actions.special_forward);
    ActionBuilder::load_from_json(*actions.special_neutral);
    ActionBuilder::load_from_json(*actions.special_up);
}
