#include "editor/ActionEditor.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Algorithms.hpp>
#include <sqee/app/GuiWidgets.hpp>

namespace algo = sq::algo;
namespace maths = sq::maths;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_hurtblobs()
{
    if (mActiveHurtblobsContext == nullptr) return;

    HurtblobsContext& ctx = *mActiveHurtblobsContext;
    Fighter& fighter = *ctx.fighter;

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
            if (const auto [iter, ok] = fighter.hurtBlobs.try_emplace(newKey); ok)
            {
                HurtBlob& blob = iter->second;
                blob.fighter = &fighter;
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
    //for (auto& [key, blob] : fighter.hurtBlobs)
    for (auto& item : fighter.hurtBlobs)
    {
        auto& key = item.first; auto& blob = item.second;

        if (collapseAll) ImGui::SetNextItemOpen(false);
        const bool sectionOpen = ImGui::CollapsingHeader(key, ImGuiTreeNodeFlags_AllowItemOverlap);

        const ImPlus::ScopeID blobIdScope = { key };

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
            ImPlus::Text("Delete '%s'?"_fmt_(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
        });

        ImPlus::if_Popup("rename_hurtblob", 0, [&]()
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

        ImPlus::if_Popup("copy_hurtblob", 0, [&]()
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

            ImPlus::InputVector(" OriginA", blob.originA, 0, "%.5f");
            ImPlus::InputVector(" OriginB", blob.originB, 0, "%.5f");
            ImPlus::SliderValue(" Radius", blob.radius, 0.05f, 1.5f, "%.3f");

            const auto& boneNames = fighter.get_armature().get_bone_names();
            if (ImPlus::Button(" # ") && blob.bone >= 0)
                blob.originA = blob.originB = fighter.get_armature().get_rest_pose().at(blob.bone).offset;
            ImPlus::HoverTooltip("snap origins to bone");
            ImGui::SameLine();
            ImPlus::Combo(" Bone", boneNames, blob.bone, "(None)");
        }
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
    {
        const auto iter = fighter.hurtBlobs.find(*toDelete);
        SQASSERT(iter != fighter.hurtBlobs.end(), "");
        fighter.hurtBlobs.erase(iter);
        ctx.hiddenKeys.erase(*toDelete);
    }

    if (toRename.has_value() == true)
    {
        const auto iter = fighter.hurtBlobs.find(toRename->first);
        SQASSERT(iter != fighter.hurtBlobs.end(), "");
        if (fighter.hurtBlobs.find(toRename->second) == fighter.hurtBlobs.end())
        {
            auto node = fighter.hurtBlobs.extract(iter);
            node.key() = toRename->second;
            fighter.hurtBlobs.insert(std::move(node));
            if (ctx.hiddenKeys.erase(toRename->first) > 0u)
                ctx.hiddenKeys.emplace(toRename->second);
        }
    }

    if (toCopy.has_value() == true)
    {
        const auto iter = fighter.hurtBlobs.find(toCopy->first);
        SQASSERT(iter != fighter.hurtBlobs.end(), "");
        fighter.hurtBlobs.try_emplace(toCopy->second, iter->second);
    }
}
