#include "main/DebugGui.hpp"

#include "game/Action.hpp"
#include "game/Fighter.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

void DebugGui::show_widget_fighter(Fighter& fighter)
{
    if (!ImPlus::CollapsingHeader("Fighter {} - {}"_format(fighter.index, fighter.type),
                                  ImGuiTreeNodeFlags_DefaultOpen)) return;

    const ImPlus::ScopeID scopeId = fighter.index;

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

        ImPlus::Text("Position: {:+.6f}"_format(fighter.current.position));
        ImPlus::HoverTooltip("Previous: {:+.6f}"_format(fighter.previous.position));

        // todo: nicer rotation display, although rotation could probably just be a float not a quat
        ImPlus::Text("Rotation: {:+.3f}"_format(fighter.current.rotation));
        ImPlus::HoverTooltip("Previous: {:+.3f}"_format(fighter.previous.rotation));

        ImPlus::Text("State: {}"_format(fighter.status.state));
        ImPlus::Text("Facing: {}"_format(fighter.status.facing));
        ImPlus::Text("Velocity: {:+.6f}"_format(fighter.status.velocity));
        ImPlus::Text("Damage: {}%"_format(fighter.status.damage));

        ImPlus::Text("Translate: {:+.6f}"_format(fighter.mTranslate));

        // todo: because the gui renders after we update, this shows values for next
        // frame, rather than what is actually on the screen, which is confusing

        const char* const animation = [&]() {
            if (fighter.mAnimation) return fighter.mAnimation->key.c_str();
            return "null"; }();

        const char* const nextAnimation = [&]() {
            if (fighter.mNextAnimation) return fighter.mNextAnimation->key.c_str();
            if (fighter.mAnimation && fighter.mAnimation->anim.looping()) return "loop";
            return "null"; }();

        const uint animTotalTime = fighter.mAnimation ? fighter.mAnimation->anim.totalTime : 0u;

        ImPlus::Text("animation: {} -> {}"_format(animation, nextAnimation));
        ImPlus::Text("animation time: {} / {}"_format(fighter.mAnimTimeDiscrete, animTotalTime));
        ImPlus::Text("animation fade: {} / {}"_format(fighter.mFadeProgress, fighter.mFadeFrames));

        if (fighter.mActiveAction == nullptr) ImPlus::Text("action: None");
        else ImPlus::Text("action: {} ({})"_format(fighter.mActiveAction->type, fighter.mActiveAction->mCurrentFrame));
    }

    if (ImGui::CollapsingHeader("Edit Stats"))
    {
        const ImPlus::ScopeFont font = ImPlus::FONT_MONO;
        const ImPlus::ScopeItemWidth width = 160.f;

        ImPlus::InputValue("walk_speed",    fighter.stats.walk_speed,    0.001f, "%.6f");
        ImPlus::InputValue("dash_speed",    fighter.stats.dash_speed,    0.001f, "%.6f");
        ImPlus::InputValue("air_speed",     fighter.stats.air_speed,     0.001f, "%.6f");
        ImPlus::InputValue("traction",      fighter.stats.traction,      0.001f, "%.6f");
        ImPlus::InputValue("air_mobility",  fighter.stats.air_mobility,  0.001f, "%.6f");
        ImPlus::InputValue("air_friction",  fighter.stats.air_friction,  0.001f, "%.6f");
        ImPlus::InputValue("hop_height",    fighter.stats.hop_height,    0.001f, "%.6f");
        ImPlus::InputValue("jump_height",   fighter.stats.jump_height,   0.001f, "%.6f");
        ImPlus::InputValue("airhop_height", fighter.stats.airhop_height, 0.001f, "%.6f");
        ImPlus::InputValue("gravity",       fighter.stats.gravity,       0.001f, "%.6f");
        ImPlus::InputValue("fall_speed",    fighter.stats.fall_speed,    0.001f, "%.6f");
        ImPlus::InputValue("weight",        fighter.stats.weight,        0.001f, "%.6f");

        ImPlus::InputValue("extra_jumps", fighter.stats.extra_jumps, 1u);

        ImPlus::InputValue("land_heavy_min_time", fighter.stats.land_heavy_min_time, 1u);

        ImPlus::InputValue("dash_start_time",  fighter.stats.dash_start_time,  1u);
        ImPlus::InputValue("dash_brake_time",  fighter.stats.dash_brake_time,  1u);
        ImPlus::InputValue("dash_turn_time",   fighter.stats.dash_turn_time,   1u);
        ImPlus::InputValue("ledge_climb_time", fighter.stats.ledge_climb_time, 1u);

        ImPlus::InputValue("anim_walk_stride", fighter.stats.anim_walk_stride, 0.01f, "%.4f");
        ImPlus::InputValue("anim_dash_stride", fighter.stats.anim_dash_stride, 0.01f, "%.4f");
    }

    // todo: this isn't very useful coz it doesn't show commands that get consumed
    if (ImGui::CollapsingHeader("Input Commands"))
    {
        for (int i = 0; i != 8; ++i)
        {
            ImPlus::Text("T-{}: "_format(i));
            for (Fighter::Command cmd : fighter.mCommands[i])
            {
                ImGui::SameLine();
                ImPlus::Text(sq::enum_to_string(cmd));
            }
        }
    }

    if (ImGui::Button("RESET"))
    {
        fighter.current.position = { 0.f, 0.f };
    }
    ImPlus::HoverTooltip("reset the fighter's position");

    ImGui::SameLine();

    if (ImGui::Button("BOUNCE"))
    {
        fighter.status.velocity.y = +10.f;
    }
    ImPlus::HoverTooltip("make the fighter bounce");
}
