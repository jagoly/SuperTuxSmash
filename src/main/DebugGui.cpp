#include "main/DebugGui.hpp"

#include "game/Controller.hpp"
#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/Stage.hpp"
#include "game/World.hpp"

#include "render/Renderer.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

void DebugGui::show_widget_fighter(Fighter& fighter)
{
    const ImPlus::ScopeID scopeId = fighter.index;

    const ImGuiStyle& style = ImGui::GetStyle();
    {
        ImGui::SetCursorPosX(style.WindowPadding.x * 0.5f + 1.f);
        if (ImGui::Button("RESET")) fighter.pass_boundary();
        ImPlus::HoverTooltip("reset the fighter");
    }
    ImGui::SameLine();

    const auto flags = fighter.index == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0;
    if (!ImPlus::CollapsingHeader(format("Fighter {} - {}", fighter.index, fighter.def.name), flags))
        return;

    //--------------------------------------------------------//

    auto& attrs = fighter.attributes;
    auto& diamond = fighter.localDiamond;
    auto& vars = fighter.variables;

    const auto action = fighter.activeAction;
    const auto state = fighter.activeState;

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

        if (state != nullptr)
        {
            if (vars.freezeTime != 0u)
                ImPlus::Text(format("State: {} (frozen for {})", state->def.name, vars.freezeTime));

            else if (state->def.name.ends_with("Stun"))
                ImPlus::Text(format("State: {} (stunned for {})", state->def.name, vars.stunTime));

            else ImPlus::Text(format("State: {}", state->def.name));
        }
        else ImPlus::Text("State: None");

        if (action != nullptr)
        {
            ImPlus::Text(format("Action: {} ({})", action->def.name, action->mCurrentFrame - 1u));
        }
        else ImPlus::Text("Action: None");

        ImPlus::Text(format("Translate: {:+.3f}", fighter.current.translation));
        ImPlus::HoverTooltipFixed(format("Previous: {:+.3f}", fighter.previous.translation));

        ImPlus::Text(format("Rotate: {:+.2f}", fighter.current.rotation));
        ImPlus::HoverTooltipFixed(format("Previous: {:+.2f}\nMode:     {:05b}", fighter.previous.rotation, uint8_t(fighter.mRotateMode)));

        ImPlus::Text(format("Pose: {}", fighter.debugCurrentPoseInfo));
        ImPlus::HoverTooltipFixed(format("Previous: {}\nFade:     {}", fighter.debugPreviousPoseInfo, fighter.debugAnimationFadeInfo));
    }

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("Edit Variables"))
    {
        const ImPlus::ScopeItemWidth width = -120.f;

        ImPlus::DragVector("position", vars.position, 0.01f, "%+.4f");
        ImPlus::DragVector("velocity", vars.velocity, 0.01f, "%+.4f");

        int8_t facingTemp = vars.facing;
        ImPlus::SliderValue("facing", facingTemp, -1, +1, "%+d");
        if (facingTemp != 0) vars.facing = facingTemp;

        ImPlus::SliderValue("extraJumps",    vars.extraJumps,    0, attrs.extraJumps);
        ImPlus::SliderValue("lightLandTime", vars.lightLandTime, 0, attrs.lightLandTime);
        ImPlus::SliderValue("noCatchTime",   vars.noCatchTime,   0, 48);

        ImPlus::DragValue("stunTime",   vars.stunTime,   1, 0, 255);
        ImPlus::DragValue("freezeTime", vars.freezeTime, 1, 0, 255);

        ImPlus::ComboEnum("edgeStop", vars.edgeStop);

        ImPlus::Checkbox("intangible",    &vars.intangible);    ImGui::SameLine(150.f);
        ImPlus::Checkbox("fastFall",      &vars.fastFall);
        ImPlus::Checkbox("applyGravity",  &vars.applyGravity);  ImGui::SameLine(150.f);
        ImPlus::Checkbox("applyFriction", &vars.applyFriction);
        ImPlus::Checkbox("flinch",        &vars.flinch);        ImGui::SameLine(150.f);
        ImPlus::Checkbox("onGround",      &vars.onGround);
        ImPlus::Checkbox("onPlatform",    &vars.onPlatform);

        int8_t edgeTemp = vars.edge;
        ImPlus::SliderValue("edge", edgeTemp, -1, +1, "%+d");
        if (edgeTemp != 0) vars.edge = edgeTemp;

        ImPlus::DragValue("moveMobility", vars.moveMobility, 0.0001f, 0, 0, "%.4f");
        ImPlus::DragValue("moveSpeed",    vars.moveSpeed,    0.001f,  0, 0, "%.4f");

        ImPlus::SliderValue("damage",      vars.damage,      0.f, 300.f,         "%.2f");
        ImPlus::SliderValue("shield",      vars.shield,      0.f, SHIELD_MAX_HP, "%.2f");
        ImPlus::SliderValue("launchSpeed", vars.launchSpeed, 0.f, 100.f,         "%.4f");

        ImPlus::DragValue("animTime", vars.animTime, 0.0001f, 0.f, 0.f, "%.4f");

        ImPlus::DragVector("attachPoint", vars.position, 0.01f, "%+.4f");

        ImPlus::LabelText("ledge", format("{}", static_cast<void*>(vars.ledge)));
    }

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("Edit Attributes"))
    {
        const ImPlus::ScopeItemWidth width = -150.f;

        ImPlus::InputValue("walkSpeed",      attrs.walkSpeed,      0.1f, "%.4f");
        ImPlus::InputValue("dashSpeed",      attrs.dashSpeed,      0.1f, "%.4f");
        ImPlus::InputValue("airSpeed",       attrs.airSpeed,       0.1f, "%.4f");
        ImPlus::InputValue("traction",       attrs.traction,       0.1f, "%.4f");
        ImPlus::InputValue("airMobility",    attrs.airMobility,    0.1f, "%.4f");
        ImPlus::InputValue("airFriction",    attrs.airFriction,    0.1f, "%.4f");
        ImGui::Separator();
        ImPlus::InputValue("hopHeight",      attrs.hopHeight,      0.1f, "%.4f");
        ImPlus::InputValue("jumpHeight",     attrs.jumpHeight,     0.1f, "%.4f");
        ImPlus::InputValue("airHopHeight",   attrs.airHopHeight,   0.1f, "%.4f");
        ImPlus::InputValue("gravity",        attrs.gravity,        0.1f, "%.4f");
        ImPlus::InputValue("fallSpeed",      attrs.fallSpeed,      0.1f, "%.4f");
        ImPlus::InputValue("fastFallSpeed",  attrs.fastFallSpeed,  0.1f, "%.4f");
        ImPlus::InputValue("weight",         attrs.weight,         1.f,  "%.1f");
        ImGui::Separator();
        ImPlus::InputValue("walkAnimSpeed",  attrs.walkAnimSpeed,  0.1f, "%.4f");
        ImPlus::InputValue("dashAnimSpeed",  attrs.dashAnimSpeed,  0.1f, "%.4f");
        ImGui::Separator();
        ImPlus::InputValue("extraJumps",     attrs.extraJumps,     1,    "%d");
        ImPlus::InputValue("lightLandTime",  attrs.lightLandTime,  1,    "%d");
        ImGui::Separator();

        bool diamondChange = false;
        diamondChange |= ImPlus::InputValue("diamondHalfWidth",   diamond.halfWidth,   0.1f, "%.4f");
        diamondChange |= ImPlus::InputValue("diamondOffsetCross", diamond.offsetCross, 0.1f, "%.4f");
        diamondChange |= ImPlus::InputValue("diamondOffsetTop",   diamond.offsetTop,   0.1f, "%.4f");
        if (diamondChange) diamond.compute_normals();
    }

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("Input History"))
    {
        Controller& controller = *fighter.controller;

        if (ImGui::Button("Load..."))
            {} // todo

        ImGui::SameLine();
        if (ImGui::Button("Save..."))
            {} // todo

        // todo: this is unintuitive, it would be better if the swap fighters button just didn't swap recordings
        ImGui::SameLine();
        ImGui::Button("Copy...");
        ImPlus::if_PopupContextItem("copy_recording", ImGuiPopupFlags_MouseButtonLeft, [&]()
        {
            for (auto& other : fighter.world.get_fighters())
            {
                if (ImPlus::MenuItem(format("Fighter {}", other->index), nullptr, false, other.get() != &fighter))
                    other->controller->mRecordedInput = controller.mRecordedInput;
            }
        });

        ImGui::SameLine();
        if (ImGui::Button("Discard"))
            controller.mRecordedInput.clear();

        if (ImGui::RadioButton("Store", controller.mPlaybackIndex == -2))
            controller.mPlaybackIndex = -2;

        ImGui::SameLine();
        if (ImGui::RadioButton("Record", controller.mPlaybackIndex == -1))
            controller.mPlaybackIndex = -1;

        ImGui::SameLine();
        if (ImGui::RadioButton("Play", controller.mPlaybackIndex >= 0))
            controller.mPlaybackIndex = 0;

        ImGui::SameLine();
        ImPlus::Text(format(" frames: {}", controller.mRecordedInput.size()));

        // todo: show current input state
    }
}

//============================================================================//

void DebugGui::show_widget_stage(Stage& stage)
{
    if (!ImPlus::CollapsingHeader(format("Stage - {}", stage.name), 0))
        return;

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const ImPlus::ScopeItemWidth width = 160.f;

        auto& tonemap = stage.world.renderer.tonemap;

        ImPlus::SliderValue("Exposure", tonemap.exposure, 0.25f, 4.f);
        ImPlus::SliderValue("Contrast", tonemap.contrast, 0.5f,  2.f);
        ImPlus::SliderValue("Black",    tonemap.black,    0.5f,  2.f);
    }
}
