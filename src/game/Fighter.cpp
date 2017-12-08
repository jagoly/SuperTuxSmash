#include <sqee/assert.hpp>
#include <sqee/debug/Logging.hpp>

#include <sqee/app/GuiWidgets.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/maths/Functions.hpp>

#include "game/private/PrivateFighter.hpp"

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
    const float angle = maths::radians(hit.knockAngle * float(hit.fighter->facing));
    const Vec2F knockDir = { std::sin(angle), std::cos(angle) };

    mVelocity = maths::normalize(knockDir) * hit.knockBase;

    state = State::Knocked;
}

void Fighter::pass_boundary()
{
    impl->current.position = Vec2F();
}

//============================================================================//

Mat4F Fighter::interpolate_model_matrix(float blend) const
{
    const Vec2F position = maths::mix(impl->previous.position, impl->current.position, blend);
    const QuatF rotation = QuatF(0.f, 0.25f * float(facing), 0.f);
    return maths::transform(Vec3F(position, 0.f), rotation, Vec3F(1.f));
}

std::vector<Mat34F> Fighter::interpolate_bone_matrices(float blend) const
{
    auto blendPose = impl->armature.blend_poses(impl->previous.pose, impl->current.pose, blend);
    return impl->armature.compute_ubo_data(blendPose);
}

//============================================================================//

void Fighter::impl_show_fighter_widget()
{
    const auto widget = gui::scope_collapse("Fighter %d - %s"_fmt_(index, mName), {}, gui::FONT_BOLD);

    if (widget.want_display() == false) return;

    //--------------------------------------------------------//

    WITH (gui::scope_framed_group())
    {
        const auto font = gui::scope_font(gui::FONT_MONO);

        gui::display_text("Position: %s"_fmt_(impl->current.position));
        gui::display_text("Velocity: %s"_fmt_(mVelocity));

        gui::display_text("state: %s"_fmt_(enum_to_string(state)));
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
