#include "editor/EditorScene.hpp" // IWYU pragma: associated
#include "editor/Editor_Helpers.hpp"

#include "game/Emitter.hpp"
#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"

#include <sqee/app/GuiWidgets.hpp>

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_emitters()
{
    if (mDoResetDockEmitters) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockEmitters = false;

    const ImPlus::ScopeWindow window = { "Emitters", 0 };
    if (window.show == false) return;

    ActionContext& ctx = *mActiveActionContext;
    FighterAction& action = *ctx.action;
    const sq::Armature& armature = ctx.fighter->get_armature();

    //--------------------------------------------------------//

    const auto funcInit = [&](Emitter& emitter)
    {
        emitter.fighter = ctx.fighter;
    };

    const auto funcEdit = [&](Emitter& emitter)
    {
        const ImPlus::ScopeItemWidth widthScope = -100.f;

        ImPlus::InputVector(" Origin", emitter.origin, 0, "%.4f");

        ImPlus::Combo(" Bone", armature.get_bone_names(), emitter.bone, "(None)");

        ImPlus::SliderValue(" Count", emitter.count, 0u, 120u, "%u");

        ImPlus::InputVector(" Velocity", emitter.velocity, 0, "%.4f");

        ImGui::InputText(" Sprite", emitter.sprite.data(), emitter.sprite.capacity());

        if (emitter.colour.empty()) emitter.colour = { Vec3F(1.f, 1.f, 1.f) };
        int indexToDelete = -1;
        for (int index = 0; index < emitter.colour.size(); ++index)
        {
            const ImPlus::ScopeID idScope = index;
            if (ImGui::Button(" X ")) indexToDelete = index;
            ImGui::SameLine();
            ImPlus::InputColour("RGB (Linear)", emitter.colour[index], ImGuiColorEditFlags_Float);
        }
        if (indexToDelete >= 0)
            emitter.colour.erase(emitter.colour.begin() + indexToDelete);
        if (ImGui::Button("Add New Entry") && !emitter.colour.full())
            emitter.colour.emplace_back();

        ImPlus::SliderValue(" BaseOpacity", emitter.baseOpacity, 0.2f, 1.f, "%.3f");
        ImPlus::SliderValue(" EndOpacity", emitter.endOpacity, 0.f, 5.f, "%.3f");
        ImPlus::SliderValue(" EndScale", emitter.endScale, 0.f, 5.f, "%.3f");

        ImPlus::DragValueRange2(" Lifetime", emitter.lifetime.min, emitter.lifetime.max, 1.f/8, 4u, 240u, "%d");
        ImPlus::DragValueRange2(" BaseRadius", emitter.baseRadius.min, emitter.baseRadius.max, 1.f/320, 0.05f, 1.f, "%.3f");

        ImPlus::DragValueRange2(" BallOffset", emitter.ballOffset.min, emitter.ballOffset.max, 1.f/80, 0.f, 2.f, "%.3f");
        ImPlus::DragValueRange2(" BallSpeed", emitter.ballSpeed.min, emitter.ballSpeed.max, 1.f/80, 0.f, 10.f, "%.3f");

        ImPlus::DragValueRange2(" DiscIncline", emitter.discIncline.min, emitter.discIncline.max, 1.f/320, -0.25f, 0.25f, "%.3f");
        ImPlus::DragValueRange2(" DiscOffset", emitter.discOffset.min, emitter.discOffset.max, 1.f/80, 0.f, 2.f, "%.3f");
        ImPlus::DragValueRange2(" DiscSpeed", emitter.discSpeed.min, emitter.discSpeed.max, 1.f/80, 0.f, 10.f, "%.3f");
    };

    //--------------------------------------------------------//

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    helper_edit_objects(action.mEmitters, funcInit, funcEdit);
}
