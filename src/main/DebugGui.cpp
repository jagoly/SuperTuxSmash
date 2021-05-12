#include "main/DebugGui.hpp"

#include "game/Action.hpp"
#include "game/Controller.hpp"
#include "game/Fighter.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

void DebugGui::show_widget_fighter(Fighter& fighter)
{
    const ImPlus::ScopeID scopeId = fighter.index;

    const ImGuiStyle& style = ImGui::GetStyle();
    {
        const ImPlus::ScopeUnindent indent = style.WindowPadding.x * 0.5f - 1.f;
        if (ImGui::Button("RESET")) fighter.status = Fighter::Status();
        ImPlus::HoverTooltip("reset the fighter's status");
    }
    ImGui::SameLine(0.f, style.ItemSpacing.x + style.WindowPadding.x * 0.5f - 1.f);

    const auto flags = fighter.index == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0;
    if (!ImPlus::CollapsingHeader("Fighter {} - {}"_format(fighter.index, fighter.type), flags))
        return;

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

        //ImPlus::Text("Translation: {:+.4f}"_format(fighter.current.translation));
        //ImPlus::HoverTooltip("Previous: {:+.4f}"_format(fighter.previous.translation));

        //ImPlus::Text("Rotation: {:+.3f}"_format(fighter.current.rotation));
        //ImPlus::HoverTooltip("Previous: {:+.3f}"_format(fighter.previous.rotation));

        ImPlus::Text("Position: {:+.6f}"_format(fighter.status.position));
        ImPlus::Text("Velocity: {:+.6f}"_format(fighter.status.velocity));
        ImPlus::Text("Facing: {}"_format(fighter.status.facing));
        ImPlus::Text("Damage: {:.2f}"_format(fighter.status.damage));
        ImPlus::Text("Shield: {:.2f}"_format(fighter.status.shield));

        //ImPlus::Text("Translate: {:+.6f}"_format(fighter.mTranslate));

        // show the progress and total time for some states
        SWITCH (fighter.status.state) {

        CASE (Freeze)
        {
            ImPlus::Text("State: {} ({}/{})"_format(fighter.status.state, fighter.mStateProgress, fighter.mFreezeTime));
            ImPlus::HoverTooltip("FrozenState: {} | FrozenProgress: {}"_format(fighter.mFrozenState, fighter.mFrozenProgress));
        }

        CASE (Stun, AirStun, TumbleStun)
            ImPlus::Text("State: {} ({}/{})"_format(fighter.status.state, fighter.mStateProgress, fighter.mHitStunTime));

        CASE (PreJump)
            ImPlus::Text("State: {} ({}/{})"_format(fighter.status.state, fighter.mStateProgress, JUMP_DELAY));

        CASE_DEFAULT ImPlus::Text("State: {}"_format(fighter.status.state));

        } SWITCH_END;

        // todo: because the gui renders after we update, this shows values for next
        // frame, rather than what is actually on the screen, which is confusing

        const char* const animation = [&]() {
            if (fighter.mAnimation) return fighter.mAnimation->key.c_str();
            return "null"; }();

        const char* const nextAnimation = [&]() {
            if (fighter.mNextAnimation) return fighter.mNextAnimation->key.c_str();
            if (fighter.mAnimation && fighter.mAnimation->is_looping()) return "loop";
            return "null"; }();

        const uint animTotalTime = fighter.mAnimation ? fighter.mAnimation->anim.frameCount : 0u;

        ImPlus::Text("animation: {} -> {}"_format(animation, nextAnimation));
        ImPlus::Text("animation time: {} / {}"_format(fighter.mAnimTimeDiscrete, animTotalTime));
        ImPlus::Text("animation fade: {} / {}"_format(fighter.mFadeProgress, fighter.mFadeFrames));

        if (fighter.mActiveAction == nullptr) ImPlus::Text("action: None");
        else ImPlus::Text("action: {} ({})"_format(fighter.mActiveAction->type, fighter.mActiveAction->mCurrentFrame));
    }

    if (ImGui::CollapsingHeader("Edit Attributes"))
    {
        const ImPlus::ScopeFont font = ImPlus::FONT_MONO;
        const ImPlus::ScopeItemWidth width = 150.f;

        ImPlus::InputValue("walk_speed",     fighter.attributes.walk_speed,     0.1f, "%.6f");
        ImPlus::InputValue("dash_speed",     fighter.attributes.dash_speed,     0.1f, "%.6f");
        ImPlus::InputValue("air_speed",      fighter.attributes.air_speed,      0.1f, "%.6f");
        ImPlus::InputValue("traction",       fighter.attributes.traction,       0.1f, "%.6f");
        ImPlus::InputValue("air_mobility",   fighter.attributes.air_mobility,   0.1f, "%.6f");
        ImPlus::InputValue("air_friction",   fighter.attributes.air_friction,   0.1f, "%.6f");
        ImPlus::InputValue("hop_height",     fighter.attributes.hop_height,     0.1f, "%.6f");
        ImPlus::InputValue("jump_height",    fighter.attributes.jump_height,    0.1f, "%.6f");
        ImPlus::InputValue("airhop_height",  fighter.attributes.airhop_height,  0.1f, "%.6f");
        ImPlus::InputValue("gravity",        fighter.attributes.gravity,        0.1f, "%.6f");
        ImPlus::InputValue("fall_speed",     fighter.attributes.fall_speed,     0.1f, "%.6f");
        ImPlus::InputValue("fastfall_speed", fighter.attributes.fastfall_speed, 0.1f, "%.6f");
        ImPlus::InputValue("weight",         fighter.attributes.weight,         1.f,  "%.5f");

        ImPlus::InputValue("extra_jumps", fighter.attributes.extra_jumps, 1u);

        ImPlus::InputValue("land_heavy_fall_time", fighter.attributes.land_heavy_fall_time, 1u);

        ImPlus::InputValue("dash_start_time",  fighter.attributes.dash_start_time,  1u);
        ImPlus::InputValue("dash_brake_time",  fighter.attributes.dash_brake_time,  1u);
        ImPlus::InputValue("dash_turn_time",   fighter.attributes.dash_turn_time,   1u);

        ImPlus::InputValue("anim_walk_stride", fighter.attributes.anim_walk_stride, 0.1f, "%.3f");
        ImPlus::InputValue("anim_dash_stride", fighter.attributes.anim_dash_stride, 0.1f, "%.3f");

        bool diamondChange = false;
        diamondChange |= ImPlus::InputValue("diamond_half_width",   fighter.mLocalDiamond.halfWidth,   0.1f, "%.3f");
        diamondChange |= ImPlus::InputValue("diamond_offset_cross", fighter.mLocalDiamond.offsetCross, 0.1f, "%.3f");
        diamondChange |= ImPlus::InputValue("diamond_offset_top",   fighter.mLocalDiamond.offsetTop,   0.1f, "%.3f");
        if (diamondChange) fighter.mLocalDiamond.compute_normals();
    }

    // don't show this section at all in the editor
    if (fighter.mController != nullptr && ImGui::CollapsingHeader("Input Commands"))
    {
        if (ImPlus::RadioButton("Record", fighter.mController->mPlaybackIndex == -1))
            fighter.mController->mPlaybackIndex = -1;

        ImGui::SameLine();

        if (ImPlus::RadioButton("Play", fighter.mController->mPlaybackIndex >= 0))
            fighter.mController->mPlaybackIndex = 0;

        for (int i = 0; i != 8; ++i)
        {
            ImPlus::Text("T-{}: "_format(i));
            for (FighterCommand cmd : fighter.mCommands[i])
            {
                ImGui::SameLine();
                if (uint8_t(cmd) & 128u)
                {
                    const ImPlus::ScopeFont bold = ImPlus::FONT_BOLD;
                    ImPlus::Text(sq::enum_to_string(FighterCommand(uint8_t(cmd) & ~128u)));
                }
                else
                {
                    const ImPlus::ScopeFont italic = ImPlus::FONT_ITALIC;
                    ImPlus::Text(sq::enum_to_string(cmd));
                }
            }
        }
    }
}
