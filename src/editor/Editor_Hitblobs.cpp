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

    const auto boneMats = fighter.get_armature().compute_skeleton_matrices(fighter.get_armature().get_rest_pose());
    const auto& boneNames = fighter.get_armature().get_bone_names();

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

            if (ImPlus::Button(" # ##1") && blob.bone >= 0)
            {
                mTempVec3F = Vec3F(maths::inverse(boneMats[blob.bone]) * Vec4F(blob.origin, 1.f));
                ImGui::OpenPopup("input_bone_origin");
            }
            ImPlus::HoverTooltip("input origin in bone space");
            ImGui::SameLine();
            ImPlus::InputVector(" Origin", blob.origin, 0, "%.4f");

            ImPlus::SliderValue(" Radius", blob.radius, BLOB_RADIUS_MIN, BLOB_RADIUS_MAX, "%.3f metres");

            if (ImPlus::Button(" #  ##2") && blob.bone >= 0)
                blob.origin = Vec3F(boneMats[blob.bone][3]);
            ImPlus::HoverTooltip("snap origin to bone");
            ImGui::SameLine();
            ImPlus::Combo(" Bone", boneNames, blob.bone, "(None)");

            ImPlus::InputValue(" Group", blob.group, 1, "%u");
            ImPlus::InputValue(" KnockAngle", blob.knockAngle, 0.01f, "%.4f");
            ImPlus::InputValue(" KnockBase", blob.knockBase, 0.01f, "%.4f");
            ImPlus::InputValue(" KnockScale", blob.knockScale, 0.01f, "%.4f");
            ImPlus::InputValue(" Damage", blob.damage, 0.01f, "%.4f");
            ImPlus::ComboEnum(" Flavour", blob.flavour);
            ImPlus::ComboEnum(" Priority", blob.priority);

            ImPlus::if_Popup("input_bone_origin", 0, [&]()
            {
                ImPlus::InputVector("", mTempVec3F, 0, "%.4f");
                if (ImPlus::Button("Confirm"))
                {
                    blob.origin = Vec3F(boneMats[blob.bone] * Vec4F(mTempVec3F, 1.f));
                    ImGui::CloseCurrentPopup();
                }
            });
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
