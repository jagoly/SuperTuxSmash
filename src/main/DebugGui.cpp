#include "main/DebugGui.hpp"

#include "game/Article.hpp"
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
    IMPLUS_WITH(Scope_ID) = fighter.index;

    const ImGuiStyle& style = ImGui::GetStyle();
    {
        ImGui::SetCursorPosX(style.WindowPadding.x * 0.5f + 1.f);
        if (ImGui::Button("RESET")) fighter.pass_boundary();
        ImPlus::HoverTooltip("reset the fighter");
    }
    ImGui::SameLine();

    const auto flags = fighter.index == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0;
    if (!ImGui::CollapsingHeader(fmt::format("Fighter {} - {}", fighter.index, fighter.def.name), flags))
        return;

    //--------------------------------------------------------//

    auto& attrs = fighter.attributes;
    auto& vars = fighter.variables;

    const auto action = fighter.activeAction;
    const auto state = fighter.activeState;

    const auto& armature = fighter.def.armature;

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        IMPLUS_WITH(Scope_Font) = ImPlus::FONT_MONO;

        if (state != nullptr)
        {
            if (vars.freezeTime != 0u)
                ImPlus::Text(fmt::format("State: {} (frozen for {})", state->def.name, vars.freezeTime));

            else if (state->def.name.ends_with("Stun"))
                ImPlus::Text(fmt::format("State: {} (stunned for {})", state->def.name, vars.stunTime));

            else ImPlus::Text(fmt::format("State: {}", state->def.name));
        }
        else ImPlus::Text("State: None");

        if (action != nullptr)
        {
            ImPlus::Text(fmt::format("Action: {} ({})", action->def.name, action->mCurrentFrame - 1u));
        }
        else ImPlus::Text("Action: None");

        ImPlus::Text(fmt::format("Translate: {:+.3f}", fighter.current.translation));
        ImPlus::HoverTooltip(true, ImGuiDir_Left, "Previous: {:+.3f}", fighter.previous.translation);

        ImPlus::Text(fmt::format("Rotate: {:+.2f}", fighter.current.rotation));
        ImPlus::HoverTooltip(true, ImGuiDir_Left, "Previous: {:+.2f}\nMode:     {:05b}", fighter.previous.rotation, uint8_t(fighter.mRotateMode));

        ImPlus::Text(fmt::format("Pose: {}", fighter.debugCurrentPoseInfo));
        // todo: this tooltip (and only this tooltip) flickers and I have no idea why
        ImPlus::HoverTooltip(true, ImGuiDir_Left, "Previous: {}\nFade:     {}", fighter.debugPreviousPoseInfo, fighter.debugAnimationFadeInfo);

        // todo: make this work for articles, and add a seperate widget for non-bone animation blocks
        const auto display_bone = [&](auto& display_bone, size_t bone) -> void
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            const bool hasChildren = ranges::any_of(armature.get_bone_infos(), [&](const auto& info) { return info.parent == int8_t(bone); });
            const bool open = ImGui::TreeNodeEx (
                fmt::format("{:>2}  {}", bone, armature.get_bone_names()[bone]),
                ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth | (hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet)
            );

            const auto& c = reinterpret_cast<sq::Armature::Bone*>(fighter.mAnimPlayer.currentSample.data())[bone];
            const auto& p = reinterpret_cast<sq::Armature::Bone*>(fighter.mAnimPlayer.previousSample.data())[bone];
            const auto& m = reinterpret_cast<Mat34F*>(fighter.world.renderer.ubos.matrices.map_only())[fighter.mAnimPlayer.modelMatsIndex + 1u + bone];

            ImPlus::HoverTooltip (
                true, ImGuiDir_Left,
                "Offset\n  C {:+.3f}\n  P {:+.3f}\nRotation\n  C {:+.2f}\n  P {:+.2f}\nScale\n  C {:+.3f}\n  P {:+.3f}\nMatrix\n"
                "  ({:+.4f}, {:+.4f}, {:+.4f}, {:+.4f})\n  ({:+.4f}, {:+.4f}, {:+.4f}, {:+.4f})\n  ({:+.4f}, {:+.4f}, {:+.4f}, {:+.4f})",
                c.offset, p.offset, c.rotation, p.rotation, c.scale, p.scale,
                m[0][0], m[0][1], m[0][2], m[0][3], m[1][0], m[1][1], m[1][2], m[1][3], m[2][0], m[2][1], m[2][2], m[2][3]
            );

            ImGui::TableNextColumn();
            ImGui::Checkbox(fmt::format("##{}", bone), reinterpret_cast<bool*>(&fighter.mAnimPlayer.debugEnableBlend[bone]));
            ImPlus::HoverTooltip("blend with previous transform");
            if (open)
            {
                if (hasChildren)
                    for (size_t child = 0u; child < armature.get_bone_count(); ++child)
                        if (armature.get_bone_infos()[child].parent == int8_t(bone))
                            display_bone(display_bone, child);
                ImGui::TreePop();
            }
        };

        ImPlus::if_Table("", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit, ImVec2(), 0.f, [&]()
        {
            IMPLUS_WITH(Style_IndentSpacing) = 10.f;
            IMPLUS_WITH(Style_SelectableTextAlign) = { 0.f, 0.5f };

            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.f);
            ImGui::TableSetupColumn("", 0, 0.f);

            for (size_t bone = 0u; bone < armature.get_bone_count(); ++bone)
                if (armature.get_bone_infos()[bone].parent == -1)
                    display_bone(display_bone, bone);
        });
    }

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("Edit Variables"))
    {
        IMPLUS_WITH(Scope_ItemWidth) = -130.f;

        ImPlus::DragVector("position", vars.position, 0.01f, "%+.4f");
        ImPlus::DragVector("velocity", vars.velocity, 0.01f, "%+.4f");

        int8_t facingTemp = vars.facing;
        ImPlus::SliderValue("facing", facingTemp, -1, +1, "%+d");
        if (facingTemp != 0) vars.facing = facingTemp;

        ImPlus::SliderValue("extraJumps",    vars.extraJumps,    0, attrs.extraJumps);
        ImPlus::SliderValue("lightLandTime", vars.lightLandTime, 0, attrs.lightLandTime);
        ImPlus::SliderValue("noCatchTime",   vars.noCatchTime,   0, 48);

        ImPlus::DragValue("freezeTime",  vars.freezeTime,  1, 0, 255);
        ImPlus::DragValue("stunTime",    vars.stunTime,    1, 0, 255);
        ImPlus::DragValue("reboundTime", vars.reboundTime, 1, 0, 255);

        ImPlus::ComboEnum("edgeStop", vars.edgeStop);

        ImGui::Checkbox("intangible",    &vars.intangible);    ImPlus::AutoArrange(150.f);
        ImGui::Checkbox("invincible",    &vars.invincible);    ImPlus::AutoArrange(150.f);
        ImGui::Checkbox("fastFall",      &vars.fastFall);      ImPlus::AutoArrange(150.f);
        ImGui::Checkbox("applyGravity",  &vars.applyGravity);  ImPlus::AutoArrange(150.f);
        ImGui::Checkbox("applyFriction", &vars.applyFriction); ImPlus::AutoArrange(150.f);
        ImGui::Checkbox("flinch",        &vars.flinch);        ImPlus::AutoArrange(150.f);
        ImGui::Checkbox("grabable",      &vars.grabable);      ImPlus::AutoArrange(150.f);
        ImGui::Checkbox("onGround",      &vars.onGround);      ImPlus::AutoArrange(150.f);
        ImGui::Checkbox("onPlatform",    &vars.onPlatform);

        int8_t edgeTemp = vars.edge;
        ImPlus::SliderValue("edge", edgeTemp, -1, +1, "%+d");
        if (edgeTemp != 0) vars.edge = edgeTemp;

        ImPlus::DragValue("moveMobility", vars.moveMobility, 0.0001f, 0, 0, "%.4f");
        ImPlus::DragValue("moveSpeed",    vars.moveSpeed,    0.001f,  0, 0, "%.4f");

        ImPlus::SliderValue("damage",      vars.damage,      0.f, 300.f,         "%.2f");
        ImPlus::SliderValue("shield",      vars.shield,      0.f, SHIELD_MAX_HP, "%.2f");
        ImPlus::SliderValue("launchSpeed", vars.launchSpeed, 0.f, 100.f,         "%.4f");

        ImPlus::LabelText("launchEntity", fmt::to_string(vars.launchEntity));

        ImPlus::DragValue("animTime", vars.animTime, 0.0001f, 0.f, 0.f, "%.4f");

        ImPlus::DragVector("attachPoint", vars.attachPoint, 0.01f, "%+.4f");

        ImPlus::LabelText("bully",  fmt::to_string(static_cast<void*>(vars.bully)));
        ImPlus::LabelText("victim", fmt::to_string(static_cast<void*>(vars.victim)));
        ImPlus::LabelText("ledge",  fmt::to_string(static_cast<void*>(vars.ledge)));
    }

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("Edit Attributes"))
    {
        IMPLUS_WITH(Scope_ItemWidth) = -130.f;

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

        ImPlus::LabelText("diamondBones", fmt::to_string(fmt::join(attrs.diamondBones, ", ")));
        ImPlus::HoverTooltip (
            true, ImGuiDir_Left, "{}",
            fmt::join(views::transform(attrs.diamondBones, [&](uint8_t bone){ return armature.get_bone_names()[bone]; }), ", ")
        );

        ImPlus::InputValue("diamondMinWidth",  attrs.diamondMinWidth,  0.1f, "%.4f");
        ImPlus::InputValue("diamondMinHeight", attrs.diamondMinHeight, 0.1f, "%.4f");
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
                if (ImGui::MenuItem(fmt::format("Fighter {}", other->index), ImStrv(), false, other.get() != &fighter))
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
        ImPlus::Text(fmt::format(" frames: {}", controller.mRecordedInput.size()));

        // todo: show current input state
    }
}

//============================================================================//

void DebugGui::show_widget_article(Article& article)
{
    IMPLUS_WITH(Scope_ID) = article.eid;

    if (!ImGui::CollapsingHeader(fmt::format("Article {} - {}", article.eid, article.def.name), 0))
        return;

    auto& vars = article.variables;

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        IMPLUS_WITH(Scope_Font) = ImPlus::FONT_MONO;

        ImPlus::Text(article.def.directory); // no label so it fits

        if (article.fighter != nullptr)
            ImPlus::Text(fmt::format("Fighter: {} ({})", article.fighter->index, article.fighter->def.name));

        else ImPlus::Text("Fighter: None");

        ImPlus::Text(fmt::format("Translate: {:+.3f}", article.current.translation));
        ImPlus::HoverTooltip(true, ImGuiDir_Left, "Previous: {:+.3f}", article.previous.translation);

        ImPlus::Text(fmt::format("Rotate: {:+.2f}", article.current.rotation));
        ImPlus::HoverTooltip(true, ImGuiDir_Left, "Previous: {:+.2f}\nMode:     {:05b}", article.previous.rotation, uint8_t(article.mRotateMode));

        ImPlus::Text(fmt::format("Pose: {}", article.debugCurrentPoseInfo));
        ImPlus::HoverTooltip(true, ImGuiDir_Left, "Previous: {}\nFade:     {}", article.debugPreviousPoseInfo, article.debugAnimationFadeInfo);
    }

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("Edit Variables", ImGuiTreeNodeFlags_DefaultOpen))
    {
        IMPLUS_WITH(Scope_ItemWidth) = -130.f;

        ImPlus::DragVector("position", vars.position, 0.01f, "%+.4f");
        ImPlus::DragVector("velocity", vars.velocity, 0.01f, "%+.4f");

        int8_t facingTemp = vars.facing;
        ImPlus::SliderValue("facing", facingTemp, -1, +1, "%+d");
        if (facingTemp != 0) vars.facing = facingTemp;

        ImPlus::DragValue("freezeTime", vars.freezeTime, 1, 0, 255);

        ImGui::Checkbox("bounced", &vars.bounced); ImPlus::AutoArrange(150.f);
        ImGui::Checkbox("fragile", &vars.fragile);

        ImPlus::DragValue("animTime", vars.animTime, 0.0001f, 0.f, 0.f, "%.4f");

        ImPlus::DragVector("attachPoint", vars.attachPoint, 0.01f, "%+.4f");

        ImPlus::LabelText("bully",  fmt::to_string(static_cast<void*>(vars.bully)));
        ImPlus::LabelText("victim", fmt::to_string(static_cast<void*>(vars.victim)));
    }
}

//============================================================================//

void DebugGui::show_widget_stage(Stage& stage)
{
    if (!ImGui::CollapsingHeader(fmt::format("Stage - {}", stage.name), 0))
        return;

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen))
    {
        IMPLUS_WITH(Scope_ItemWidth) = -100.f;

        auto& tonemap = stage.mEnvironment.tonemap;

        ImPlus::SliderValue("Exposure", tonemap.exposure, 0.25f, 4.f);
        ImPlus::SliderValue("Contrast", tonemap.contrast, 0.5f,  2.f);
        ImPlus::SliderValue("Black",    tonemap.black,    0.5f,  2.f);
    }
}
