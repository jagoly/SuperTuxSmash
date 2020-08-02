#include "editor/EditorScene.hpp" // IWYU pragma: associated

#include "game/Action.hpp"
#include "game/Fighter.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_script()
{
    if (mActiveActionContext == nullptr) return;

    ActionContext& ctx = *mActiveActionContext;
    //Fighter& fighter = *ctx.fighter;
    Action& action = *ctx.fighter->get_action(ctx.key.action);

    if (mDoResetDockScript) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockScript = false;

    const ImPlus::ScopeWindow window = { "Script", 0 };
    if (window.show == false) return;

    auto font = ImPlus::ScopeFont(ImPlus::FONT_MONO);

    ImGui::SetNextItemWidth(-1);
    const float widgetHeight = ImGui::GetContentRegionAvail().y;
    ImPlus::InputStringMultiline("##Script", action.mLuaSource, {0.f, widgetHeight}, ImGuiInputTextFlags_NoUndoRedo);

    /*

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    //--------------------------------------------------------//

//    if (ImGui::Button("New...")) ImGui::OpenPopup("new_procedure");

    ImGui::SameLine();
    const bool collapseAll = ImGui::Button("Collapse All");

    //--------------------------------------------------------//

    ImPlus::if_Popup("new_procedure", 0, [&]()
    {
        ImGui::TextUnformatted("Create New Procedure:");
        TinyString newKey;
        if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (action.procedures.count(newKey) == 0u)
            {
                action.procedures.emplace(newKey, Action::Procedure());
                build_working_procedures(ctx);
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
    for (auto& itemIter : ctx.sortedProcedures)
    {
        // c++20: lambda capture structured bindings
        // auto& [key, procedure] = *itemIter;
        auto& key = itemIter->first; auto& procedure = itemIter->second;

        const ImPlus::ScopeID procedureIdScope = { key };

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

        if (choice == Choice::Delete) ImGui::OpenPopup("delete_procedure");
        if (choice == Choice::Rename) ImGui::OpenPopup("rename_procedure");
        if (choice == Choice::Copy) ImGui::OpenPopup("copy_procedure");

        ImPlus::if_Popup("delete_procedure", 0, [&]()
        {
            ImPlus::Text("Delete '{}'?"_fmt_(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
        });

        ImPlus::if_Popup("rename_procedure", 0, [&]()
        {
            ImPlus::Text("Rename '{}':"_fmt_(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toRename.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        });

        ImPlus::if_Popup("copy_procedure", 0, [&]()
        {
            ImPlus::Text("Copy '{}':"_fmt_(key));
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
            // TODO: the builtin InputTextMultiline widget is meh, but good enough for now
            // may want to use something like https://github.com/BalazsJako/ImGuiColorTextEdit

            const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

            const uint numNewLines = std::count(procedure.meta.source.begin(), procedure.meta.source.end(), '\n') + 2u;
            const float inputHeight = float(numNewLines) * ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.y * 2.f;

            ImPlus::InputStringMultiline("", procedure.meta.source, {-1.f, inputHeight}, ImGuiInputTextFlags_NoUndoRedo);

            for (const String& error : ctx.buildErrors[key])
            {
                ImGui::Bullet();
                ImPlus::TextWrapped(error);
            }
        }
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
    {
        const auto iter = action.procedures.find(*toDelete);
        SQASSERT(iter != action.procedures.end(), "");
        action.procedures.erase(iter);
        build_working_procedures(ctx);
    }

    if (toRename.has_value() == true)
    {
        const auto iter = action.procedures.find(toRename->first);
        SQASSERT(iter != action.procedures.end(), "");
        if (action.procedures.count(toRename->second) == 0u)
        {
            auto node = action.procedures.extract(iter);
            node.key() = toRename->second;
            action.procedures.insert(std::move(node));
            build_working_procedures(ctx);
        }
    }

    if (toCopy.has_value() == true)
    {
        const auto iter = action.procedures.find(toCopy->first);
        SQASSERT(iter != action.procedures.end(), "");
        if (action.procedures.count(toCopy->second) == 0u)
        {
            action.procedures.emplace(toCopy->second, iter->second);
            build_working_procedures(ctx);
        }
    }*/
}

//============================================================================//

void EditorScene::impl_show_widget_timeline()
{
    if (mActiveActionContext == nullptr) return;

    ActionContext& ctx = *mActiveActionContext;
    //Fighter& fighter = *ctx.fighter;
    //Action& action = *ctx.fighter->get_action(ctx.key.action);

    if (mDoResetDockTimeline) ImGui::SetNextWindowDockID(mDockDownId);
    mDoResetDockTimeline = false;

    const ImPlus::ScopeWindow window = { "Timeline", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    //--------------------------------------------------------//

    const ImPlus::ScopeFont font = ImPlus::FONT_MONO;
    const ImPlus::Style_ButtonTextAlign align = {0.f, 0.f};

    //--------------------------------------------------------//

    const auto selectableAlign = ImPlus::Style_SelectableTextAlign(0.5f, 0.f);

    for (int i = -1; i < int(ctx.timelineLength); ++i)
    {
        ImGui::SameLine();

        const String label = "{}"_format(i);

        const bool active = ctx.currentFrame == i;
        const bool open = ImPlus::IsPopupOpen(label);

        if (ImPlus::Selectable(label, active || open, 0, {20u, 20u}))
            ImPlus::OpenPopup(label);

        if (ctx.currentFrame != i)
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImPlus::MOUSE_RIGHT))
               scrub_to_frame(ctx, i);

        const ImVec2 rectMin = ImGui::GetItemRectMin();
        const ImVec2 rectMax = ImGui::GetItemRectMax();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRect(rectMin, rectMax, ImGui::GetColorU32(ImGuiCol_Border)); // inefficient, lots of overlap

        if (i == ctx.currentFrame)
        {
            const float blend = mPreviewMode == PreviewMode::Pause ? mBlendValue : float(mAccumulation / mTickTime);
            const float middle = maths::mix(rectMin.x, rectMax.x, blend);

            const ImVec2 scrubberTop = { middle, rectMax.y + 2.f };
            const ImVec2 scrubberLeft = { middle - 3.f, scrubberTop.y + 8.f };
            const ImVec2 scrubberRight = { middle + 3.f, scrubberTop.y + 8.f };

            drawList->AddTriangleFilled(scrubberTop, scrubberLeft, scrubberRight, ImGui::GetColorU32(ImGuiCol_Border));
        }
    }
}
