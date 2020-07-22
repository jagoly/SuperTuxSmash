#include "editor/EditorScene.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/app/GuiWidgets.hpp>

namespace maths = sq::maths;
using sq::literals::operator""_fmt_;
using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_emitters()
{
    if (mActiveActionContext == nullptr) return;

    ActionContext& ctx = *mActiveActionContext;
    Fighter& fighter = *ctx.fighter;
    Action& action = *fighter.get_action(ctx.key.action);

    if (mDoResetDockEmitters) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockEmitters = false;

    const ImPlus::ScopeWindow window = { "Emitters", 0 };
    if (window.show == false) return;

    const ImPlus::ScopeID ctxKeyIdScope = ctx.key.hash();

    //--------------------------------------------------------//

    if (ImGui::Button("New...")) ImGui::OpenPopup("new_emitter");

    ImGui::SameLine();
    const bool collapseAll = ImGui::Button("Collapse All");

    //--------------------------------------------------------//

    ImPlus::if_Popup("new_emitter", 0, [&]()
    {
        ImGui::TextUnformatted("Create New Emitter:");
        TinyString newKey;
        if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (auto [iter, ok] = action.mEmitters.try_emplace(newKey); ok)
            {
                ParticleEmitter& emitter = iter->second;
                emitter.fighter = &fighter;
                emitter.action = &action;
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
    //for (auto& [key, emitter] : action.mEmitters)
    for (auto& item : action.mEmitters)
    {
        auto& key = item.first; auto& emitter = item.second;

        const ImPlus::ScopeID emitterIdScope = { key.c_str() };

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

        if (choice == Choice::Delete) ImGui::OpenPopup("delete_emitter");
        if (choice == Choice::Rename) ImGui::OpenPopup("rename_emitter");
        if (choice == Choice::Copy) ImGui::OpenPopup("copy_emitter");

        ImPlus::if_Popup("delete_emitter", 0, [&]()
        {
            ImPlus::Text("Delete '%s'?"_fmt_(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
        });

        ImPlus::if_Popup("rename_emitter", 0, [&]()
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

        ImPlus::if_Popup("copy_emitter", 0, [&]()
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

            ImPlus::InputVector(" Origin", emitter.origin, 0, "%.4f");

            const auto& boneNames = fighter.get_armature().get_bone_names();
            ImPlus::Combo(" Bone", boneNames, emitter.bone, "(None)");

            ImPlus::InputVector(" Direction", emitter.direction, 0, "%.4f");

            ImGui::InputText(" Sprite", emitter.sprite.data(), TinyString::capacity());

            ImPlus::DragValue(" EndScale", emitter.endScale, 0.f, EMIT_END_SCALE_MAX, 0.001f, "%.3f");
            ImPlus::DragValue(" EndOpacity", emitter.endOpacity, 0.f, EMIT_END_OPACITY_MAX, 0.0025f, "%.2f");

            ImPlus::DragValueRange2(" Lifetime", emitter.lifetime.min, emitter.lifetime.max, EMIT_RAND_LIFETIME_MIN, EMIT_RAND_LIFETIME_MAX, 0.5f, "%d");
            ImPlus::DragValueRange2(" Radius", emitter.radius.min, emitter.radius.max, EMIT_RAND_RADIUS_MIN, EMIT_RAND_RADIUS_MAX, 0.001f, "%.3f");
            ImPlus::DragValueRange2(" Opacity", emitter.opacity.min, emitter.opacity.max, EMIT_RAND_OPACITY_MIN, 1.f, 0.0025f, "%.2f");
            ImPlus::DragValueRange2(" Speed", emitter.speed.min, emitter.speed.max, 0.f, EMIT_RAND_SPEED_MAX, 0.001f, "%.3f");

            ImGui::Separator();

            int colourIndex = int(emitter.colour.index());
            ImPlus::Combo(" Colour", std::array { "Fixed", "Random" }, colourIndex);

            if (colourIndex == 0)
            {
                if (emitter.colour.index() != 0)
                {
                    const Vec3F defaultValue = emitter.colour_random().front();
                    emitter.colour.emplace<ParticleEmitter::FixedColour>(defaultValue);
                }

                ImGui::ColorEdit3(" RGB", emitter.colour_fixed().data, ImGuiColorEditFlags_Float);
            }

            if (colourIndex == 1)
            {
                if (emitter.colour.index() != 1)
                {
                    const Vec3F defaultValue = emitter.colour_fixed();
                    emitter.colour.emplace<ParticleEmitter::RandomColour>({defaultValue});
                }

                int indexToDelete = -1;
                for (int index = 0; index < emitter.colour_random().size(); ++index)
                {
                    const ImPlus::ScopeID idScope = index;
                    if (ImGui::Button(" X ")) indexToDelete = index;
                    ImGui::SameLine();
                    ImGui::ColorEdit3(" RGB", emitter.colour_random()[index].data);
                }
                if (indexToDelete >= 0)
                    emitter.colour_random().erase(emitter.colour_random().begin() + indexToDelete);
                if (ImGui::Button("Add New Entry") && !emitter.colour_random().full())
                    emitter.colour_random().emplace_back();
            }

            ImGui::Separator();

            int shapeIndex = int(emitter.shape.index());
            ImPlus::Combo(" Shape", std::array { "Ball", "Disc", "Ring" }, shapeIndex);

            if (shapeIndex == 0)
            {
                if (emitter.shape.index() != 0)
                    emitter.shape.emplace<ParticleEmitter::BallShape>();

                auto& shape = emitter.shape_ball();
                ImPlus::SliderValueRange2(" Speed##shape", shape.speed.min, shape.speed.max, 0.f, 100.f, "%.3f");
            }

            if (shapeIndex == 1)
            {
                if (emitter.shape.index() != 1)
                    emitter.shape.emplace<ParticleEmitter::DiscShape>();

                auto& shape = emitter.shape_disc();
                ImPlus::SliderValueRange2(" Incline##shape", shape.incline.min, shape.incline.max, -0.25f, 0.25f, "%.3f");
                ImPlus::SliderValueRange2(" Speed##shape", shape.speed.min, shape.speed.max, 0.f, 100.f, "%.3f");
            }
        }
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
    {
        const auto iter = action.mEmitters.find(*toDelete);
        SQASSERT(iter != action.mEmitters.end(), "");
        action.mEmitters.erase(iter);
    }

    if (toRename.has_value() == true)
    {
        const auto iter = action.mEmitters.find(toRename->first);
        SQASSERT(iter != action.mEmitters.end(), "");
        if (action.mEmitters.find(toRename->second) == action.mEmitters.end())
        {
            auto node = action.mEmitters.extract(iter);
            node.key() = toRename->second;
            action.mEmitters.insert(std::move(node));
        }
    }

    if (toCopy.has_value() == true)
    {
        const auto iter = action.mEmitters.find(toCopy->first);
        SQASSERT(iter != action.mEmitters.end(), "");
        action.mEmitters.try_emplace(toCopy->second, iter->second);
    }
}
