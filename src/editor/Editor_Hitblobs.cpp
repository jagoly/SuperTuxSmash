#include "editor/EditorScene.hpp" // IWYU pragma: associated

#include "game/Action.hpp"
#include "game/Blobs.hpp"
#include "game/Fighter.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

constexpr const float BLOB_RADIUS_MIN = 0.05f;
constexpr const float BLOB_RADIUS_MAX = 2.f;
constexpr const float BLOB_DAMAGE_MIN = 0.1f;
constexpr const float BLOB_DAMAGE_MAX = 50.f;
constexpr const float BLOB_KNOCK_MIN = 0.f;
constexpr const float BLOB_KNOCK_MAX = 100.f;

//============================================================================//

void EditorScene::impl_show_widget_hitblobs()
{
    if (mActiveActionContext == nullptr) return;

    ActionContext& ctx = *mActiveActionContext;
    Fighter& fighter = *ctx.fighter;
    Action& action = *fighter.get_action(ctx.key.action);

    const sq::Armature& armature = fighter.get_armature();

    if (mDoResetDockHitblobs) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockHitblobs = false;

    const ImPlus::ScopeWindow window = { "HitBlobs", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    //--------------------------------------------------------//

    if (ImGui::Button("New...")) ImGui::OpenPopup("new_hitblob");

    ImGui::SameLine();
    const bool collapseAll = ImGui::Button("Collapse All");

    //--------------------------------------------------------//

    ImPlus::if_Popup("new_hitblob", 0, [&]()
    {
        ImGui::TextUnformatted("Create New HitBlob:");
        TinyString newKey;
        if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (auto [iter, ok] = action.mBlobs.try_emplace(newKey); ok)
            {
                HitBlob& blob = iter->second;
                blob.fighter = &fighter;
                blob.action = &action;
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
    //for (auto& [key, blob] : action.mBlobs)
    for (auto& item : action.mBlobs)
    {
        auto& key = item.first; auto& blob = item.second;

        const ImPlus::ScopeID blobIdScope = { key.c_str() };

        if (collapseAll) ImGui::SetNextItemOpen(false);
        const bool sectionOpen = ImGui::CollapsingHeader(key);

        //--------------------------------------------------------//

        enum class Choice { None, Delete, Rename, Copy } choice {};

        ImPlus::if_PopupContextItem(nullptr, ImPlus::MOUSE_RIGHT, [&]()
        {
            if (ImGui::MenuItem("Delete...")) choice = Choice::Delete;
            if (ImGui::MenuItem("Rename...")) choice = Choice::Rename;
            if (ImGui::MenuItem("Copy...")) choice = Choice::Copy;
        });

        if (choice == Choice::Delete) ImGui::OpenPopup("delete_hitblob");
        if (choice == Choice::Rename) ImGui::OpenPopup("rename_hitblob");
        if (choice == Choice::Copy) ImGui::OpenPopup("copy_hitblob");

        ImPlus::if_Popup("delete_hitblob", 0, [&]()
        {
            ImPlus::Text("Delete '{}'?"_format(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
        });

        ImPlus::if_Popup("rename_hitblob", 0, [&]()
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

        ImPlus::if_Popup("copy_hitblob", 0, [&]()
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

        if (sectionOpen == true)
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

            ImPlus::SliderValue(" Radius", blob.radius, BLOB_RADIUS_MIN, BLOB_RADIUS_MAX, "%.2f metres");

            if (ImPlus::Button(" # ##2") && blob.bone >= 0)
                blob.origin = Vec3F(armature.compute_bone_matrix(armature.get_rest_pose(), blob.bone)[3]);
            ImPlus::HoverTooltip("snap origin to bone");
            ImGui::SameLine();
            ImPlus::Combo(" Bone", armature.get_bone_names(), blob.bone, "(None)");

            ImPlus::InputValue(" Group", blob.group, 1, "%u");
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
        }
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
    {
        const auto iter = action.mBlobs.find(*toDelete);
        SQASSERT(iter != action.mBlobs.end(), "");
        action.mBlobs.erase(iter);
    }

    if (toRename.has_value() == true)
    {
        const auto iter = action.mBlobs.find(toRename->first);
        SQASSERT(iter != action.mBlobs.end(), "");
        if (action.mBlobs.find(toRename->second) == action.mBlobs.end())
        {
            auto node = action.mBlobs.extract(iter);
            node.key() = toRename->second;
            action.mBlobs.insert(std::move(node));
        }
    }

    if (toCopy.has_value() == true)
    {
        const auto iter = action.mBlobs.find(toCopy->first);
        SQASSERT(iter != action.mBlobs.end(), "");
        action.mBlobs.try_emplace(toCopy->second, iter->second);
    }
}
