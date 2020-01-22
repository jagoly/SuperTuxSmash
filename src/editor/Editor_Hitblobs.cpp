#include "editor/ActionEditor.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/app/GuiWidgets.hpp>

namespace maths = sq::maths;
using sq::literals::operator""_fmt_;
using namespace sts;

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
            if (auto [iter, ok] = action.blobs.try_emplace(newKey); ok)
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

    Optional<TinyString> toDelete;
    Optional<Pair<TinyString, TinyString>> toRename;
    Optional<Pair<TinyString, TinyString>> toCopy;

    //--------------------------------------------------------//

    // c++20: lambda capture structured bindings
    //for (auto& [key, blob] : action.blobs)
    for (auto& item : action.blobs)
    {
        auto& key = item.first; auto& blob = item.second;

        const ImPlus::ScopeID blobIdScope = { key };

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
            ImPlus::Text("Delete '%s'?"_fmt_(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
        });

        ImPlus::if_Popup("rename_hitblob", 0, [&]()
        {
            ImPlus::Text("Rename '%s':"_fmt_(key));
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
            ImPlus::Text("Copy '%s':"_fmt_(key));
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

            ImPlus::InputVector(" Origin", blob.origin, 0, "%.4f");
            ImPlus::SliderValue(" Radius", blob.radius, 0.05f, 2.0f, "%.3f metres");

            const auto& boneNames = fighter.get_armature().get_bone_names();
            if (ImPlus::Button(" # ") && blob.bone >= 0)
                blob.origin = fighter.get_armature().get_rest_pose().at(blob.bone).offset;
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
        }
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
    {
        const auto iter = action.blobs.find(*toDelete);
        SQASSERT(iter != action.blobs.end(), "");
        action.blobs.erase(iter);
    }

    if (toRename.has_value() == true)
    {
        const auto iter = action.blobs.find(toRename->first);
        SQASSERT(iter != action.blobs.end(), "");
        if (action.blobs.find(toRename->second) == action.blobs.end())
        {
            auto node = action.blobs.extract(iter);
            node.key() = toRename->second;
            action.blobs.insert(std::move(node));
        }
    }

    if (toCopy.has_value() == true)
    {
        const auto iter = action.blobs.find(toCopy->first);
        SQASSERT(iter != action.blobs.end(), "");
        action.blobs.try_emplace(toCopy->second, iter->second);
    }
}
