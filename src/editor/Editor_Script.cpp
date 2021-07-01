#include "editor/EditorScene.hpp" // IWYU pragma: associated

#include "game/Action.hpp"
#include "game/Fighter.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

constexpr const float MAX_HEIGHT_ERRORS = 160;

//============================================================================//

void EditorScene::impl_show_widget_script()
{
    if (mDoResetDockScript) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockScript = false;

    const ImPlus::ScopeWindow window { "Script", 0 };
    if (window.show == false) return;

    ActionContext& ctx = *mActiveActionContext;
    Action& action = *ctx.fighter->get_action(ctx.key.action);

    //--------------------------------------------------------//

    const ImPlus::ScopeFont font { ImPlus::FONT_MONO };

    const ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    const ImVec2 inputSize = { contentRegion.x, contentRegion.y - MAX_HEIGHT_ERRORS };

    ImPlus::InputStringMultiline("##Script", action.mWrenSource, inputSize, ImGuiInputTextFlags_NoUndoRedo);

    ImPlus::TextWrapped(action.mErrorMessage);
}

//============================================================================//

void EditorScene::impl_show_widget_timeline()
{
    if (mDoResetDockTimeline) ImGui::SetNextWindowDockID(mDockDownId);
    mDoResetDockTimeline = false;

    const ImPlus::ScopeWindow window { "Timeline", ImGuiWindowFlags_AlwaysHorizontalScrollbar };
    if (window.show == false) return;

    ActionContext& ctx = *mActiveActionContext;

    //--------------------------------------------------------//

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

    const ImPlus::Style_ButtonTextAlign buttonAlign = {0.f, 0.f};
    const ImPlus::Style_SelectableTextAlign selectableAlign = {0.5f, 0.f};

    //--------------------------------------------------------//

    // todo: can this be done better using imgui's tables?

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

        drawList->AddRect(rectMin, rectMax, ImGui::GetColorU32(ImGuiCol_Border));

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
