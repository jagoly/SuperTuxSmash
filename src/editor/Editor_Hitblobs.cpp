#include "editor/EditorScene.hpp" // IWYU pragma: associated
#include "editor/Editor_Helpers.hpp"

#include "game/Action.hpp"
#include "game/Fighter.hpp"
#include "game/HitBlob.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_hitblobs()
{
    if (mDoResetDockHitblobs) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockHitblobs = false;

    const ImPlus::ScopeWindow window = { "HitBlobs", 0 };
    if (window.show == false) return;

    ActionContext& ctx = *mActiveActionContext;
    Action& action = *ctx.fighter->get_action(ctx.key.action);
    const sq::Armature& armature = ctx.fighter->get_armature();

    //--------------------------------------------------------//

    const auto funcInit = [&](HitBlob& blob)
    {
        blob.action = &action;
    };

    const auto funcEdit = [&](HitBlob& blob)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        static Vec3F localOrigin;

        if (ImPlus::Button(" # ##1") && blob.bone >= 0)
        {
            const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_pose(), blob.bone);
            localOrigin = Vec3F(maths::inverse(boneMatrix) * Vec4F(blob.origin, 1.f));
            ImGui::OpenPopup("input_local_origin");
        }
        ImPlus::HoverTooltip("input value in bone local space");
        ImGui::SameLine();
        ImPlus::InputVector(" Origin", blob.origin, 0, "%.4f");

        ImPlus::if_Popup("input_local_origin", 0, [&]()
        {
            ImPlus::InputVector("", localOrigin, 0, "%.4f");
            const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_pose(), blob.bone);
            const Vec3F newValue = Vec3F(boneMatrix * Vec4F(localOrigin, 1.f));
            if (std::abs(newValue.x - blob.origin.x) > 0.000001f) blob.origin.x = newValue.x;
            if (std::abs(newValue.y - blob.origin.y) > 0.000001f) blob.origin.y = newValue.y;
            if (std::abs(newValue.z - blob.origin.z) > 0.000001f) blob.origin.z = newValue.z;
        });

        ImPlus::SliderValue(" Radius", blob.radius, 0.05f, 2.f, "%.2f metres");

        if (ImPlus::Button(" # ##2") && blob.bone >= 0)
            blob.origin = Vec3F(armature.compute_bone_matrix(armature.get_rest_pose(), blob.bone)[3]);
        ImPlus::HoverTooltip("snap origin to bone");
        ImGui::SameLine();
        ImPlus::Combo(" Bone", armature.get_bone_names(), blob.bone, "(None)");

        ImPlus::InputValue(" Index", blob.index, 1, "%u");

        ImPlus::InputValue(" Damage", blob.damage, 1.f, "%.2f %");
        ImPlus::InputValue(" FreezeFactor", blob.freezeFactor, 0.1f, "%.2f ×");
        ImPlus::InputValue(" KnockAngle", blob.knockAngle, 1.f, "%.2f degrees");
        ImPlus::InputValue(" KnockBase", blob.knockBase, 1.f, "%.2f units");
        ImPlus::InputValue(" KnockScale", blob.knockScale, 1.f, "%.2f units");

        ImPlus::ComboEnum(" Facing", blob.facing);
        ImPlus::ComboEnum(" Flavour", blob.flavour);

        ImPlus::Checkbox("FixedKnockback ", &blob.useFixedKnockback);
        ImGui::SameLine();
        ImPlus::Checkbox("SakuraiAngle ", &blob.useSakuraiAngle);

        // todo: make this a combo box, or at least show a warning if the sound doesn't exist
        ImGui::InputText(" Sound", blob.sound.data(), blob.sound.buffer_size());
    };

    //--------------------------------------------------------//

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    helper_edit_objects(action.mBlobs, funcInit, funcEdit);
}
