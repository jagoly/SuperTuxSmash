#include "main/DebugGui.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Logging.hpp>

#include "game/Fighter.hpp"
#include "game/private/PrivateFighter.hpp"

using namespace sts;

using sq::literals::operator""_fmt_;

//============================================================================//

void DebugGui::show_widget_fighter(Fighter& fighter)
{
    const String label = "Fighter %d - %s"_fmt_(fighter.index, fighter.type);
    if (ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen) == false) return;

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

        ImPlus::Text("Position: %s"_fmt_(fighter.impl->current.position));
        ImPlus::HoverTooltip("Previous: %s"_fmt_(fighter.impl->previous.position));
        ImPlus::Text("Rotation: %s"_fmt_(fighter.impl->current.rotation)); // todo: this line is too long
        ImPlus::HoverTooltip("Previous: %s"_fmt_(fighter.impl->previous.rotation));

        ImPlus::Text("state: %s"_fmt_(fighter.current.state));
        ImPlus::HoverTooltip("Previous: %s"_fmt_(fighter.previous.state));
        ImPlus::Text("facing: %d"_fmt_(fighter.current.facing));
        ImPlus::HoverTooltip("Previous: %d"_fmt_(fighter.previous.facing));

        ImPlus::Text("Velocity: %s"_fmt_(fighter.mVelocity));
        ImPlus::Text("Translate: %s"_fmt_(fighter.mTranslate));
        ImPlus::Text("Damage: %0.f%%"_fmt_(fighter.status.damage));

        const char* const animation = [&]() {
            if (fighter.impl->mAnimation) return fighter.impl->mAnimation->key.c_str();
            return "null"; }();

        const char* const nextAnimation = [&]() {
            if (fighter.impl->mNextAnimation) return fighter.impl->mNextAnimation->key.c_str();
            if (fighter.impl->mAnimation && fighter.impl->mAnimation->anim.looping()) return "loop";
            return "null"; }();

        const uint animTotalTime = fighter.impl->mAnimation ? fighter.impl->mAnimation->anim.totalTime : 0u;

        ImPlus::Text("animation: %s -> %s"_fmt_(animation, nextAnimation));
        ImPlus::Text("animation time: %d / %d"_fmt_(fighter.impl->mAnimTimeDiscrete, animTotalTime));
        ImPlus::Text("animation fade: %d / %d"_fmt_(fighter.impl->mFadeProgress, fighter.impl->mFadeFrames));

        if (fighter.current.action == nullptr) ImPlus::Text("action: None");
        else ImPlus::Text("action: %s (%d)"_fmt_(fighter.current.action->type, fighter.current.action->mCurrentFrame));
    }

    if (ImGui::CollapsingHeader("Edit Stats"))
    {
        const ImPlus::ScopeFont font = ImPlus::FONT_MONO;
        const ImPlus::ScopeItemWidth width = 160.f;

        ImPlus::InputValue("walk_speed",     fighter.stats.walk_speed,     0.05f, "%.6f");
        ImPlus::InputValue("dash_speed",     fighter.stats.dash_speed,     0.05f, "%.6f");
        ImPlus::InputValue("air_speed",      fighter.stats.air_speed,      0.05f, "%.6f");
        ImPlus::InputValue("traction",       fighter.stats.traction,       0.05f, "%.6f");
        ImPlus::InputValue("air_mobility",   fighter.stats.air_mobility,   0.05f, "%.6f");
        ImPlus::InputValue("air_friction",   fighter.stats.air_friction,   0.05f, "%.6f");
        ImPlus::InputValue("hop_height",     fighter.stats.hop_height,     0.05f, "%.6f");
        ImPlus::InputValue("jump_height",    fighter.stats.jump_height,    0.05f, "%.6f");
        ImPlus::InputValue("air_hop_height", fighter.stats.air_hop_height, 0.05f, "%.6f");
        ImPlus::InputValue("gravity",        fighter.stats.gravity,        0.05f, "%.6f");
        ImPlus::InputValue("fall_speed",     fighter.stats.fall_speed,     0.05f, "%.6f");

        ImGui::Separator();

        ImPlus::InputValue("extra_jumps", fighter.stats.extra_jumps, 1u);

        ImPlus::InputValue("dash_start_time",  fighter.stats.dash_start_time,  1u);
        ImPlus::InputValue("dash_brake_time",  fighter.stats.dash_brake_time,  1u);
        ImPlus::InputValue("dash_turn_time",   fighter.stats.dash_turn_time,   1u);
        ImPlus::InputValue("ledge_climb_time", fighter.stats.ledge_climb_time, 1u);

        ImPlus::InputValue("anim_walk_stride", fighter.stats.anim_walk_stride, 0.1f);
        ImPlus::InputValue("anim_dash_stride", fighter.stats.anim_dash_stride, 0.1f);

        ImPlus::InputValue("land_heavy_min_time", fighter.stats.land_heavy_min_time, 1u);
    }

    if (ImGui::CollapsingHeader("Input Commands"))
    {
        for (int i = 0; i != 8; ++i)
        {
            ImPlus::Text("T-%d: "_fmt_(i));
            for (Fighter::Command cmd : fighter.mCommands[i])
            {
                ImGui::SameLine();
                ImPlus::Text(sq::enum_to_string(cmd));
            }
        }
    }

    //--------------------------------------------------------//

    if (ImGui::Button("RESET"))
    {
        fighter.impl->current.position = { 0.f, 1.f };
    }
    ImPlus::HoverTooltip("reset the fighter's position");

    ImGui::SameLine();

    if (ImGui::Button("BOUNCE"))
    {
        fighter.mVelocity.y = +10.f;
    }
    ImPlus::HoverTooltip("make the fighter bounce");
}
