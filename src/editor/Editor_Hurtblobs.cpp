#include "editor/EditorScene.hpp" // IWYU pragma: associated

#include "editor/EditorHelpers.hpp"

#include "game/Fighter.hpp"
#include "game/HurtBlob.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_hurtblobs()
{
    if (mActiveHurtblobsContext == nullptr) return;

    if (mDoResetDockHurtblobs) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockHurtblobs = false;

    const ImPlus::ScopeWindow window = { "HurtBlobs", 0 };
    if (window.show == false) return;

    //--------------------------------------------------------//

    HurtblobsContext& ctx = *mActiveHurtblobsContext;
    Fighter& fighter = *ctx.fighter;

    const sq::Armature& armature = fighter.get_armature();

    const ImPlus::ScopeID ctxKeyIdScope = int(ctx.key);

    //--------------------------------------------------------//

    const auto funcInit = [&](HurtBlob& blob)
    {
        blob.fighter = &fighter;
    };

    const auto funcEdit = [&](HurtBlob& blob)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        static Vec3F localOrigin;
        static Vec3F* localOriginPtr;

        if (ImPlus::Button(" # ##1") && blob.bone >= 0)
        {
            const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_pose(), blob.bone);
            localOrigin = Vec3F(maths::inverse(boneMatrix) * Vec4F(blob.originA, 1.f));
            localOriginPtr = &blob.originA;
            ImGui::OpenPopup("input_local_origin");
        }
        ImPlus::HoverTooltip("input value in bone local space");
        ImGui::SameLine();
        ImPlus::InputVector(" OriginA", blob.originA, 0, "%.4f");

        if (ImPlus::Button(" # ##2") && blob.bone >= 0)
        {
            const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_pose(), blob.bone);
            localOrigin = Vec3F(maths::inverse(boneMatrix) * Vec4F(blob.originB, 1.f));
            localOriginPtr = &blob.originB;
            ImGui::OpenPopup("input_local_origin");
        }
        ImPlus::HoverTooltip("input value in bone local space");
        ImGui::SameLine();
        ImPlus::InputVector(" OriginB", blob.originB, 0, "%.4f");

        ImPlus::if_Popup("input_local_origin", 0, [&]()
        {
            ImPlus::InputVector("", localOrigin, 0, "%.4f");
            const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_pose(), blob.bone);
            const Vec3F newValue = Vec3F(boneMatrix * Vec4F(localOrigin, 1.f));
            if (std::abs(newValue.x - localOriginPtr->x) > 0.000001f) localOriginPtr->x = newValue.x;
            if (std::abs(newValue.y - localOriginPtr->y) > 0.000001f) localOriginPtr->y = newValue.y;
            if (std::abs(newValue.z - localOriginPtr->z) > 0.000001f) localOriginPtr->z = newValue.z;
        });

        ImPlus::SliderValue(" Radius", blob.radius, 0.05f, 1.5f, "%.2f metres");

        if (ImPlus::Button(" # ##3") && blob.bone >= 0)
        {
            const Mat4F boneMatrix = armature.compute_bone_matrix(armature.get_rest_pose(), blob.bone);
            blob.originA = blob.originB = Vec3F(boneMatrix[3]);
        }
        ImPlus::HoverTooltip("snap origins to bone");
        ImGui::SameLine();
        ImPlus::Combo(" Bone", armature.get_bone_names(), blob.bone, "(None)");

        ImPlus::ComboEnum(" Region", blob.region);
    };

    //--------------------------------------------------------//

    helper_edit_objects(fighter.mHurtBlobs, funcInit, funcEdit);
}
