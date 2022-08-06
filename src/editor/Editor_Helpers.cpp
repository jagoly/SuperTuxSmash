#include "editor/Editor_Helpers.hpp"

#include "main/DebugGui.hpp"

#include "game/Fighter.hpp"

using namespace sts;

//============================================================================//

void EditorScene::helper_edit_origin(const char* label, Fighter& fighter, int8_t bone, Vec3F& origin)
{
    const ImPlus::ScopeID idScope = label;
    const auto& armature = fighter.def.armature;
    static Vec3F localOrigin = nullptr;

    const bool openPopup = ImGui::Button("#") && bone >= 0;
    ImPlus::HoverTooltip("input in bone local space");
    ImGui::SameLine();

    if (openPopup == true)
    {
        const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_sample(), bone);
        localOrigin = Vec3F(maths::inverse(boneMatrix) * Vec4F(origin, 1.f));
        ImGui::OpenPopup("popup_input_local");
        ImVec2 popupPos = ImGui::GetCursorScreenPos();
        popupPos.y -= ImGui::GetStyle().WindowPadding.y;
        ImGui::SetNextWindowPos(popupPos);
    }

    ImPlus::if_Popup("popup_input_local", 0, [&]()
    {
        if (ImGui::Button("#")) localOrigin = Vec3F();
        ImPlus::HoverTooltip("snap to bone");
        ImGui::SameLine();

        ImPlus::InputVector("", localOrigin, 0, "%.4f");
        const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_sample(), bone);
        const Vec3F newValue = Vec3F(boneMatrix * Vec4F(localOrigin, 1.f));
        const Vec3F diff = maths::abs(newValue - origin);
        if (maths::max(diff.x, diff.y, diff.z) > 0.000001f) origin = newValue;
    });

    ImPlus::InputVector(label, origin, 0, "%.4f");
}

//============================================================================//

void EditorScene::helper_show_widget_debug(Stage* stage, Fighter* fighter)
{
    // todo: make proper widget for editing and saving attributes, maybe merge with HurtBlobs editor?

    if (mDoResetDockDebug) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockDebug = false;

    const ImPlus::ScopeWindow window = { "Debug", 0 };
    if (window.show == false) return;

    if (stage != nullptr)
        DebugGui::show_widget_stage(*stage);

    if (fighter != nullptr)
        DebugGui::show_widget_fighter(*fighter);
}
