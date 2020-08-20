#include "editor/EditorScene.hpp" // IWYU pragma: associated

#include "game/Blobs.hpp"
#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_hurtblobs()
{
    if (mActiveHurtblobsContext == nullptr) return;

    HurtblobsContext& ctx = *mActiveHurtblobsContext;
    Fighter& fighter = *ctx.fighter;

    const sq::Armature& armature = fighter.get_armature();

    if (mDoResetDockHurtblobs) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockHurtblobs = false;

    const ImPlus::ScopeWindow window = { "HurtBlobs", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyIdScope = int(ctx.key);

    //--------------------------------------------------------//

    if (ImGui::Button("New...")) ImGui::OpenPopup("new_hurtblob");

    ImGui::SameLine(); const bool collapseAll = ImGui::Button("Collapse All");
    ImGui::SameLine(); const bool hideAll = ImGui::Button("Hide All");
    ImGui::SameLine(); const bool showAll = ImGui::Button("Show All");

    //--------------------------------------------------------//

    ImPlus::if_Popup("new_hurtblob", 0, [&]()
    {
        ImGui::TextUnformatted("Create New HurtBlob:");
        TinyString newKey;
        if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (const auto [iter, ok] = fighter.mHurtBlobs.try_emplace(newKey); ok)
            {
                HurtBlob& blob = iter->second;
                blob.fighter = &fighter;
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    });

    //--------------------------------------------------------//

    std::optional<TinyString> toDelete;
    std::optional<std::pair<TinyString, TinyString>> toRename;
    std::optional<std::pair<TinyString, TinyString>> toCopy;

    //--------------------------------------------------------//

    // c++20: lambda capture structured bindings
    //for (auto& [key, blob] : fighter.mHurtBlobs)
    for (auto& item : fighter.mHurtBlobs)
    {
        auto& key = item.first; auto& blob = item.second;

        if (collapseAll) ImGui::SetNextItemOpen(false);
        const bool sectionOpen = ImGui::CollapsingHeader(key, ImGuiTreeNodeFlags_AllowItemOverlap);

        const ImPlus::ScopeID blobIdScope = { key.c_str() };

        //--------------------------------------------------------//

        enum class Choice { None, Delete, Rename, Copy } choice {};

        ImPlus::if_PopupContextItem(nullptr, ImPlus::MOUSE_RIGHT, [&]()
        {
            if (ImGui::MenuItem("Delete...")) choice = Choice::Delete;
            if (ImGui::MenuItem("Rename...")) choice = Choice::Rename;
            if (ImGui::MenuItem("Copy...")) choice = Choice::Copy;
        });

        if (choice == Choice::Delete) ImGui::OpenPopup("delete_hurtblob");
        if (choice == Choice::Rename) ImGui::OpenPopup("rename_hurtblob");
        if (choice == Choice::Copy) ImGui::OpenPopup("copy_hurtblob");

        ImPlus::if_Popup("delete_hurtblob", 0, [&]()
        {
            ImPlus::Text("Delete '{}'?"_format(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
        });

        ImPlus::if_Popup("rename_hurtblob", 0, [&]()
        {
            ImPlus::Text("Rename '{}':"_format(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toRename.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        });

        ImPlus::if_Popup("copy_hurtblob", 0, [&]()
        {
            ImPlus::Text("Copy '{}':"_format(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toCopy.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        });

        //--------------------------------------------------------//

        const bool visible = algo::any_of(ctx.world->get_hurt_blobs(), algo::pred_equal_to(&blob));

        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFrameHeight() - ImGui::GetStyle().FramePadding.x);
        const bool toggleVisible = ImGui::RadioButton("##visible", visible);

        if ((toggleVisible || hideAll) && visible == true)
        {
            ctx.world->disable_hurt_blob(&blob);
            ctx.hiddenKeys.emplace(key);
        }
        if ((toggleVisible || showAll) && visible == false)
        {
            ctx.world->enable_hurt_blob(&blob);
            ctx.hiddenKeys.erase(key);
        }

        //--------------------------------------------------------//

        if (sectionOpen == true)
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
        }
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
    {
        const auto iter = fighter.mHurtBlobs.find(*toDelete);
        SQASSERT(iter != fighter.mHurtBlobs.end(), "");
        fighter.mHurtBlobs.erase(iter);
        ctx.hiddenKeys.erase(*toDelete);
    }

    if (toRename.has_value() == true)
    {
        const auto iter = fighter.mHurtBlobs.find(toRename->first);
        SQASSERT(iter != fighter.mHurtBlobs.end(), "");
        if (fighter.mHurtBlobs.find(toRename->second) == fighter.mHurtBlobs.end())
        {
            auto node = fighter.mHurtBlobs.extract(iter);
            node.key() = toRename->second;
            fighter.mHurtBlobs.insert(std::move(node));
            if (ctx.hiddenKeys.erase(toRename->first) > 0u)
                ctx.hiddenKeys.emplace(toRename->second);
        }
    }

    if (toCopy.has_value() == true)
    {
        const auto iter = fighter.mHurtBlobs.find(toCopy->first);
        SQASSERT(iter != fighter.mHurtBlobs.end(), "");
        fighter.mHurtBlobs.try_emplace(toCopy->second, iter->second);
    }
}
