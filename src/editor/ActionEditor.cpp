#include "editor/ActionEditor.hpp"

#include "DebugGlobals.hpp"

#include "stages/TestZone_Stage.hpp"
#include "fighters/Sara_Fighter.hpp"
#include "fighters/Tux_Fighter.hpp"

#include "stages/TestZone_Render.hpp"
#include "fighters/Sara_Render.hpp"
#include "fighters/Tux_Render.hpp"

#include "render/DebugRender.hpp"

#include "game/ActionBuilder.hpp"
#include "game/private/PrivateFighter.hpp"

#include <sqee/macros.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>

#include <algorithm> // for lots of stuff

namespace algo = sq::algo;
namespace maths = sq::maths;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

constexpr const Vec2F MIN_SIZE_HITBLOBS   = { 500, 200 };
constexpr const Vec2F MAX_SIZE_HITBLOBS   = { 600, 1000 };

constexpr const Vec2F MIN_SIZE_EMITTERS   = { 500, 200 };
constexpr const Vec2F MAX_SIZE_EMITTERS   = { 600, 1000 };

constexpr const Vec2F MIN_SIZE_PROCEDURES = { 500, 200 };
constexpr const Vec2F MAX_SIZE_PROCEDURES = { 600, 1000 };

constexpr const Vec2F MIN_SIZE_TIMELINE   = { 800, 200 };
constexpr const Vec2F MAX_SIZE_TIMELINE   = { 1500, 800 };

//----------------------------------------------------------------------------//

// silly hack that lets me use structured bindings with pointers

namespace std {

template<class T>
struct tuple_size<T*> : tuple_size<T> {};

template<size_t N, class T>
auto& get(T* ptr) { return get<N>(*ptr); }

template<size_t N, class T>
struct tuple_element<N, T*> {
    using type = decltype(get<N>(declval<T*>()));
};

} // namespace std

//============================================================================//

ActionEditor::ActionEditor(SmashApp& smashApp)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    widget_main_menu.func = [this]() { impl_show_widget_main_menu(); };
    widget_hitblobs.func = [this]() { impl_show_widget_hitblobs(); };
    widget_emitters.func = [this]() { impl_show_widget_emitters(); };
    widget_procedures.func = [this]() { impl_show_widget_procedures(); };
    widget_timeline.func = [this]() { impl_show_widget_timeline(); };

    //--------------------------------------------------------//

    mFightWorld = std::make_unique<FightWorld>(GameMode::Editor);
    mRenderer = std::make_unique<Renderer>(GameMode::Editor, mSmashApp.get_options());

    //--------------------------------------------------------//

    dbg.renderBlobs = true;
    dbg.actionEditor = true;

    //--------------------------------------------------------//

    auto stage = std::make_unique<TestZone_Stage>(*mFightWorld);
    auto renderStage = std::make_unique<TestZone_Render>(*mRenderer, static_cast<TestZone_Stage&>(*stage));

    mFightWorld->set_stage(std::move(stage));
    mRenderer->add_object(std::move(renderStage));

    //--------------------------------------------------------//

    auto worldFighter = std::make_unique<Sara_Fighter>(0u, *mFightWorld);
    auto renderFighter = std::make_unique<Sara_Render>(*mRenderer, static_cast<Sara_Fighter&>(*worldFighter));

    //auto fighter = std::make_unique<Tux_Fighter>(0u, *mFightWorld);
    //auto renderFighter = std::make_unique<Tux_Render>(*mRenderer, static_cast<Tux_Fighter&>(*fighter));

    fighter = worldFighter.get();
    privateFighter = fighter->editor_get_private();

    mFightWorld->add_fighter(std::move(worldFighter));
    mRenderer->add_object(std::move(renderFighter));

    //--------------------------------------------------------//

    receiver.subscribe(mFightWorld->get_message_bus());

    SQEE_MB_BIND_METHOD(receiver, message::fighter_action_finished, handle_message);
}

ActionEditor::~ActionEditor() = default;

//============================================================================//

void ActionEditor::handle_event(sq::Event event)
{
    if (event.type == sq::Event::Type::Keyboard_Press)
    {
        if (event.data.keyboard.key == sq::Keyboard_Key::Space)
        {
            if (mPreviewMode == PreviewMode::Pause)
            {
                mFightWorld->tick();
            }
            else
            {
                mPreviewMode = PreviewMode::Pause;
            }
        }
    }

    if (event.type == sq::Event::Type::Mouse_Scroll)
    {
        auto& camera = static_cast<EditorCamera&>(mRenderer->get_camera());
        camera.update_from_scroll(event.data.scroll.delta);
    }
}

//============================================================================//

void ActionEditor::refresh_options()
{
    mRenderer->refresh_options();
}

//============================================================================//

void ActionEditor::update()
{
    if (mLiveEditEnabled == true)
    {
        const bool hasWorkingAction = mWorkingAction != nullptr;
        const bool hasWorkingChanges = hasWorkingAction && mWorkingAction->has_changes(*mReferenceAction);
        if (hasWorkingChanges == true) apply_working_changes();
    }

    if (mPreviewMode != PreviewMode::Pause)
    {
        mFightWorld->tick();
    }
}

//============================================================================//

void ActionEditor::render(double elapsed)
{
    if (ImGui::GetIO().WantCaptureMouse == false)
    {
        const Vec2I position = mSmashApp.get_input_devices().get_cursor_location(true);
        const Vec2I windowSize = Vec2I(mSmashApp.get_window().get_window_size());

        if (position.x >= 0 && position.y >= 0 && position.x < windowSize.x && position.y < windowSize.y)
        {
            const bool leftPressed = mSmashApp.get_input_devices().is_pressed(sq::Mouse_Button::Left);
            const bool rightPressed = mSmashApp.get_input_devices().is_pressed(sq::Mouse_Button::Right);

            auto& camera = static_cast<EditorCamera&>(mRenderer->get_camera());

            camera.update_from_mouse(leftPressed, rightPressed, Vec2F(position));
        }
    }

    const float accum = float(elapsed);
    const float blend = mPreviewMode == PreviewMode::Pause ? 1.f : float(mAccumulation / mTickTime);

    mRenderer->render_objects(accum, blend);

    mRenderer->resolve_multisample();

    mRenderer->render_particles(mFightWorld->get_particle_system(), accum, blend);

    auto& debugRenderer = mRenderer->get_debug_renderer();

    if (mRenderBlobsEnabled == true)
    {
        debugRenderer.render_hit_blobs(mFightWorld->get_hit_blobs());
        debugRenderer.render_hurt_blobs(mFightWorld->get_hurt_blobs());
    }

    mRenderer->finish_rendering();
}

//============================================================================//

void ActionEditor::impl_show_widget_main_menu()
{
    const bool hasWorkingAction = mWorkingAction != nullptr;
    const bool hasWorkingChanges = hasWorkingAction && mWorkingAction->has_changes(*mReferenceAction);
    const bool hasChangedActions = mChangedActions.empty() == false;

    //--------------------------------------------------------//

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Action"))
        {
            if (ImGui::MenuItem("Apply Changes", nullptr, false, hasWorkingChanges))
                apply_working_changes();
            ImGui::HoverTooltip("Apply changes from working action");

            if (ImGui::MenuItem("Revert Changes", nullptr, false, hasWorkingChanges))
                revert_working_changes();
            ImGui::HoverTooltip("Revert changes to working action");

            if (ImGui::MenuItem("Save...", nullptr, false, hasChangedActions))
                save_changed_actions();
            ImGui::HoverTooltip("Save changes to any changed actions");

            ImGui::EndMenu();
        }

        ImGui::Separator();

        {
            ActionType actionType = ctxSwitchAction ? ctxSwitchAction->actionType
                                  : mWorkingAction ? mWorkingAction->type : ActionType::None;

            const ImGui::ScopeItemWidth itemWidth = 160.f;
            if (ImGui::ComboEnum("##ActionList", actionType, ImGuiComboFlags_HeightLargest))
            {
                if (mWorkingAction == nullptr || actionType != mWorkingAction->type)
                {
                    ctxSwitchAction = CtxSwitchAction { hasWorkingChanges, actionType };
                    if (hasWorkingChanges) ImGui::OpenPopup("Discard Changes");
                }
            }
        }

        if (ImGui::CloseButton("##ClearWorkingAction"))
        {
            if (mWorkingAction != nullptr)
            {
                ctxSwitchAction = CtxSwitchAction { hasWorkingChanges, ActionType::None };
                if (hasWorkingChanges) ImGui::OpenPopup("Discard Changes");
            }
        }

        if (ctxSwitchAction != std::nullopt)
        {
            if (ctxSwitchAction->waitConfirm == false)
            {
                mWorkingAction = nullptr;
                mReferenceAction = nullptr;

                if (ctxSwitchAction->actionType != ActionType::None)
                {
                    mWorkingAction = std::make_unique<Action>(*mFightWorld, *fighter, ctxSwitchAction->actionType, false);
                    mReferenceAction = fighter->get_action(ctxSwitchAction->actionType);
                    revert_working_changes();
                }

                privateFighter->switch_action(ctxSwitchAction->actionType);
                ctxSwitchAction = std::nullopt;
            }
            else
            {
                SWITCH(ImGui::DialogConfirmation("Discard Changes", "The working action has been modified, really discard changes?"))
                {
                    CASE(Confirm) ctxSwitchAction->waitConfirm = false;
                    CASE(Cancel) ctxSwitchAction = std::nullopt;
                    CASE(None) {}
                }
                SWITCH_END;
            }
        }

        ImGui::Separator();

        ImGui::Checkbox("Render Blobs", &mRenderBlobsEnabled);
        ImGui::HoverTooltip("when enabled, action hit blobs and fighter hurt blobs are rendered");

        if (ImGui::Checkbox("Sort Procedures", &mSortProceduresEnabled))
            if (hasWorkingAction) update_sorted_procedures();
        ImGui::HoverTooltip("when enabled, sort procedures by their timeline, otherwise sort by name");

        ImGui::Checkbox("Live Edit", &mLiveEditEnabled);
        ImGui::HoverTooltip("when enabled, changes to the working action are applied immediately");

        ImGui::Separator();

        if (ImGui::RadioButton("Pause", mPreviewMode, PreviewMode::Pause)) {}
        ImGui::HoverTooltip("press space to manually tick");
        if (ImGui::RadioButton("Normal", mPreviewMode, PreviewMode::Normal)) mTickTime = 1.0 / 48.0;
        ImGui::HoverTooltip("play preview at normal speed");
        if (ImGui::RadioButton("Slow", mPreviewMode, PreviewMode::Slow)) mTickTime = 1.0 / 12.0;
        ImGui::HoverTooltip("play preview at 1/4 speed");
        if (ImGui::RadioButton("Slower", mPreviewMode, PreviewMode::Slower)) mTickTime = 1.0 / 3.0;
        ImGui::HoverTooltip("play preview at 1/16 speed");

        ImGui::Separator();
        ImGui::SetNextItemWidth(80.f);
        if (ImGui::InputValue(" RNG Seed", mRandomSeed, 1) && hasWorkingAction)
            scrub_to_frame(mReferenceAction->mCurrentFrame);

        ImGui::EndMainMenuBar();
    }

    fighter->debug_show_fighter_widget();
}

//============================================================================//

void ActionEditor::impl_show_widget_hitblobs()
{
    if (mWorkingAction == nullptr) return;
    ImGui::SetNextWindowSizeConstraints(MIN_SIZE_HITBLOBS, MAX_SIZE_HITBLOBS);
    const ImGui::ScopeWindow window = { "Edit HitBlobs", 0 };
    if (window.open == false) return;

    //--------------------------------------------------------//

    if (ImGui::Button("New...")) ImGui::OpenPopup("New HitBlob");

    ImGui::SameLine();
    const bool collapseAll = ImGui::Button("Collapse All");

    //--------------------------------------------------------//

    if (ImGui::BeginPopup("New HitBlob"))
    {
        ImGui::TextUnformatted("Create New HitBlob:");
        TinyString newKey;
        if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (!mWorkingAction->blobs.contains(newKey))
            {
                mWorkingAction->blobs.emplace(newKey, HitBlob());
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        ImGui::EndPopup();
    }

    //--------------------------------------------------------//

    Optional<TinyString> toDelete;
    Optional<Pair<TinyString, TinyString>> toRename;
    Optional<Pair<TinyString, TinyString>> toCopy;

    //--------------------------------------------------------//

    for (auto& [key, blob] : mWorkingAction->blobs)
    {
        const ImGui::ScopeID idScope = key.c_str();

        if (collapseAll) ImGui::SetNextItemOpen(false);
        const bool sectionOpen = ImGui::CollapsingHeader(key.c_str());

        //--------------------------------------------------------//

        enum class Choice { None, Delete, Rename, Copy } choice {};

        if (ImGui::BeginPopupContextItem(nullptr, ImGui::MOUSE_RIGHT))
        {
            if (ImGui::MenuItem("Delete...")) choice = Choice::Delete;
            if (ImGui::MenuItem("Rename...")) choice = Choice::Rename;
            if (ImGui::MenuItem("Copy...")) choice = Choice::Copy;
            ImGui::EndPopup();
        }

        if (choice == Choice::Delete) ImGui::OpenPopup("Delete HitBlob");
        if (choice == Choice::Rename) ImGui::OpenPopup("Rename HitBlob");
        if (choice == Choice::Copy) ImGui::OpenPopup("Copy HitBlob");

        if (ImGui::BeginPopup("Delete HitBlob"))
        {
            ImGui::Text("Delete '%s'?"_fmt_(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Rename HitBlob"))
        {
            ImGui::Text("Rename '%s':"_fmt_(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toRename.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Copy HitBlob"))
        {
            ImGui::Text("Copy '%s':"_fmt_(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toCopy.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::EndPopup();
        }

        //--------------------------------------------------------//

        if (sectionOpen == true)
        {
            const ImGui::ScopeItemWidth widthScope = -100.f;

            ImGui::InputVector(" Origin", blob->origin, 0, "%.4f");
            ImGui::SliderValue(" Radius", blob->radius, 0.05f, 2.0f, "%.3f metres");
            ImGui::InputValue(" Group", blob->group, 1, "%u");
            ImGui::InputValue(" KnockAngle", blob->knockAngle, 0.01f, "%.4f");
            ImGui::InputValue(" KnockBase", blob->knockBase, 0.01f, "%.4f");
            ImGui::InputValue(" KnockScale", blob->knockScale, 0.01f, "%.4f");
            ImGui::InputValue(" Damage", blob->damage, 0.01f, "%.4f");
            ImGui::ComboEnum(" Flavour", blob->flavour);
            ImGui::ComboEnum(" Priority", blob->priority);
        }
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
    {
        SQASSERT(mWorkingAction->blobs.contains(*toDelete), "");
        mWorkingAction->blobs.erase(*toDelete);
    }

    if (toRename.has_value() == true)
    {
        SQASSERT(mWorkingAction->blobs.contains(toRename->first), "");
        if (!mWorkingAction->blobs.contains(toRename->second))
            mWorkingAction->blobs.modify_key(toRename->first, toRename->second);
    }

    if (toCopy.has_value() == true)
    {
        SQASSERT(mWorkingAction->blobs.contains(toCopy->first), "");
        auto sourcePtr = mWorkingAction->blobs[toCopy->first];
        if (!mWorkingAction->blobs.contains(toCopy->second))
            mWorkingAction->blobs.emplace(toCopy->second, *sourcePtr);
    }
}

//============================================================================//

void ActionEditor::impl_show_widget_emitters()
{
    if (mWorkingAction == nullptr) return;
    ImGui::SetNextWindowSizeConstraints(MIN_SIZE_EMITTERS, MAX_SIZE_EMITTERS);
    const ImGui::ScopeWindow window = { "Edit Emitters", 0 };
    if (window.open == false) return;

    //--------------------------------------------------------//

    if (ImGui::Button("New...")) ImGui::OpenPopup("New Emitter");

    ImGui::SameLine();
    const bool collapseAll = ImGui::Button("Collapse All");

    //--------------------------------------------------------//

    if (ImGui::BeginPopup("New Emitter"))
    {
        ImGui::TextUnformatted("Create New Emitter:");
        TinyString newKey;
        if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (!mWorkingAction->emitters.contains(newKey))
            {
                mWorkingAction->emitters.emplace(newKey, ParticleEmitter());
                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        ImGui::EndPopup();
    }

    //--------------------------------------------------------//

    Optional<TinyString> toDelete;
    Optional<Pair<TinyString, TinyString>> toRename;
    Optional<Pair<TinyString, TinyString>> toCopy;

    //--------------------------------------------------------//

    for (auto& [key, emitter] : mWorkingAction->emitters)
    {
        const ImGui::ScopeID idScope = key.c_str();

        if (collapseAll) ImGui::SetNextItemOpen(false);
        const bool sectionOpen = ImGui::CollapsingHeader(key.c_str());

        //--------------------------------------------------------//

        enum class Choice { None, Delete, Rename, Copy } choice {};

        if (ImGui::BeginPopupContextItem(nullptr, ImGui::MOUSE_RIGHT))
        {
            if (ImGui::MenuItem("Delete...")) choice = Choice::Delete;
            if (ImGui::MenuItem("Rename...")) choice = Choice::Rename;
            if (ImGui::MenuItem("Copy...")) choice = Choice::Copy;
            ImGui::EndPopup();
        }

        if (choice == Choice::Delete) ImGui::OpenPopup("Delete Emitter");
        if (choice == Choice::Rename) ImGui::OpenPopup("Rename Emitter");
        if (choice == Choice::Copy) ImGui::OpenPopup("Copy Emitter");

        if (ImGui::BeginPopup("Delete Emitter"))
        {
            ImGui::Text("Delete '%s'?"_fmt_(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Rename Emitter"))
        {
            ImGui::Text("Rename '%s':"_fmt_(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toRename.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Copy Emitter"))
        {
            ImGui::Text("Copy '%s':"_fmt_(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toCopy.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::EndPopup();
        }

        //--------------------------------------------------------//

        if (sectionOpen == true)
        {
            const ImGui::ScopeItemWidth widthScope = -100.f;

            ImGui::InputVector(" Origin", emitter->origin, 0, "%.4f");

            const auto& boneNames = fighter->get_armature().get_bone_names();
            ImGui::ComboPlus(" Bone", boneNames, emitter->bone, "(None)");

            ImGui::InputVector(" Direction", emitter->direction, 0, "%.4f");

            ImGui::InputText(" Sprite", emitter->sprite.data(), TinyString::capacity());

            ImGui::DragValue(" EndScale", emitter->endScale, 0.f, EMIT_END_SCALE_MAX, 0.001f, "%.3f");
            ImGui::DragValue(" EndOpacity", emitter->endOpacity, 0.f, EMIT_END_OPACITY_MAX, 0.0025f, "%.2f");

            ImGui::DragValueRange2(" Lifetime", emitter->lifetime.min, emitter->lifetime.max, EMIT_RAND_LIFETIME_MIN, EMIT_RAND_LIFETIME_MAX, 0.5f, "%d");
            ImGui::DragValueRange2(" Radius", emitter->radius.min, emitter->radius.max, EMIT_RAND_RADIUS_MIN, EMIT_RAND_RADIUS_MAX, 0.001f, "%.3f");
            ImGui::DragValueRange2(" Opacity", emitter->opacity.min, emitter->opacity.max, EMIT_RAND_OPACITY_MIN, 1.f, 0.0025f, "%.2f");
            ImGui::DragValueRange2(" Speed", emitter->speed.min, emitter->speed.max, 0.f, EMIT_RAND_SPEED_MAX, 0.001f, "%.3f");

            ImGui::Separator();

            constexpr const Array<const char*, 2> colourTypes = { "Fixed", "Random" };
            int colourIndex = emitter->colour.index();
            ImGui::ComboPlus(" Colour", colourTypes, colourIndex);

            if (colourIndex == 0)
            {
                if (emitter->colour.index() != 0)
                {
                    const Vec3F defaultValue = emitter->colour_random().front();
                    emitter->colour.emplace<ParticleEmitter::FixedColour>(defaultValue);
                }

                ImGui::ColorEdit3(" RGB", emitter->colour_fixed().data, ImGuiColorEditFlags_Float);
            }

            if (colourIndex == 1)
            {
                if (emitter->colour.index() != 1)
                {
                    const Vec3F defaultValue = emitter->colour_fixed();
                    emitter->colour.emplace<ParticleEmitter::RandomColour>({defaultValue});
                }

                int indexToDelete = -1;
                for (int index = 0; index < emitter->colour_random().size(); ++index)
                {
                    const ImGui::ScopeID idScope = index;
                    if (ImGui::Button(" X ")) indexToDelete = index;
                    ImGui::SameLine();
                    ImGui::ColorEdit3(" RGB", emitter->colour_random()[index].data);
                }
                if (indexToDelete >= 0)
                    emitter->colour_random().erase(emitter->colour_random().begin() + indexToDelete);
                if (ImGui::Button("Add New Entry") && !emitter->colour_random().full())
                    emitter->colour_random().emplace_back();
            }

            ImGui::Separator();

            constexpr const Array<const char*, 3> shapeTypes = { "Ball", "Disc", "Ring" };
            int shapeIndex = emitter->shape.index();
            ImGui::ComboPlus(" Shape", shapeTypes, shapeIndex);

            if (shapeIndex == 0)
            {
                if (emitter->shape.index() != 0)
                    emitter->shape.emplace<ParticleEmitter::BallShape>();

                ImGui::SliderValueRange2(" Speed", emitter->shape_ball().speed.min, emitter->shape_ball().speed.max, 0.f, 100.f, "%.3f");
            }

            if (shapeIndex == 1)
            {
                if (emitter->shape.index() != 1)
                    emitter->shape.emplace<ParticleEmitter::DiscShape>();

                ImGui::SliderValueRange2(" Incline", emitter->shape_disc().incline.min, emitter->shape_disc().incline.max, -0.25f, 0.25f, "%.3f");
                ImGui::SliderValueRange2(" Speed##shape", emitter->shape_disc().speed.min, emitter->shape_disc().speed.max, 0.f, 100.f, "%.3f");
            }
        }
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
    {
        SQASSERT(mWorkingAction->emitters.contains(*toDelete), "");
        mWorkingAction->emitters.erase(*toDelete);
    }

    if (toRename.has_value() == true)
    {
        SQASSERT(mWorkingAction->emitters.contains(toRename->first), "");
        if (!mWorkingAction->emitters.contains(toRename->second))
            mWorkingAction->emitters.modify_key(toRename->first, toRename->second);
    }

    if (toCopy.has_value() == true)
    {
        SQASSERT(mWorkingAction->emitters.contains(toCopy->first), "");
        auto sourcePtr = mWorkingAction->emitters[toCopy->first];
        if (!mWorkingAction->emitters.contains(toCopy->second))
            mWorkingAction->emitters.emplace(toCopy->second, *sourcePtr);
    }
}

//============================================================================//

void ActionEditor::impl_show_widget_procedures()
{
    if (mWorkingAction == nullptr) return;
    ImGui::SetNextWindowSizeConstraints(MIN_SIZE_PROCEDURES, MAX_SIZE_PROCEDURES);
    const ImGui::ScopeWindow window = { "Edit Procedures", 0 };
    if (window.open == false) return;

    //--------------------------------------------------------//

    if (ImGui::Button("New...")) ImGui::OpenPopup("New Procedure");

    ImGui::SameLine();
    const bool collapseAll = ImGui::Button("Collapse All");

    //--------------------------------------------------------//

    if (ImGui::BeginPopup("New Procedure"))
    {
        ImGui::TextUnformatted("Create New Procedure:");
        TinyString newKey;
        if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (!mWorkingAction->procedures.count(newKey))
            {
                mWorkingAction->procedures.emplace(newKey, Action::Procedure());
                update_sorted_procedures();

                ImGui::CloseCurrentPopup();
            }
        }
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        ImGui::EndPopup();
    }

    //--------------------------------------------------------//

    Optional<TinyString> toDelete;
    Optional<Pair<TinyString, TinyString>> toRename;
    Optional<Pair<TinyString, TinyString>> toCopy;

    //--------------------------------------------------------//

    for (auto& [key, procedure] : mSortedProcedures)
    {
        const ImGui::ScopeID idScope = key.c_str();

        if (collapseAll) ImGui::SetNextItemOpen(false);
        const bool sectionOpen = ImGui::CollapsingHeader(key.c_str());

        //--------------------------------------------------------//

        enum class Choice { None, Delete, Rename, Copy } choice {};

        if (ImGui::BeginPopupContextItem(nullptr, ImGui::MOUSE_RIGHT))
        {
            if (ImGui::MenuItem("Delete...")) choice = Choice::Delete;
            if (ImGui::MenuItem("Rename...")) choice = Choice::Rename;
            if (ImGui::MenuItem("Copy...")) choice = Choice::Copy;
            ImGui::EndPopup();
        }

        if (choice == Choice::Delete) ImGui::OpenPopup("Delete Procedure");
        if (choice == Choice::Rename) ImGui::OpenPopup("Rename Procedure");
        if (choice == Choice::Copy) ImGui::OpenPopup("Copy Procedure");

        if (ImGui::BeginPopup("Delete Procedure"))
        {
            ImGui::Text("Delete '%s'?"_fmt_(key));
            if (ImGui::Button("Confirm"))
            {
                toDelete = key;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Rename Procedure"))
        {
            ImGui::Text("Rename '%s':"_fmt_(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toRename.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Copy Procedure"))
        {
            ImGui::Text("Copy '%s':"_fmt_(key));
            TinyString newKey = key;
            if (ImGui::InputText("", newKey.data(), sizeof(TinyString), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                toCopy.emplace(key, newKey);
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::EndPopup();
        }

        //--------------------------------------------------------//

        if (sectionOpen == true)
        {
            const ImGui::ScopeFont font = ImGui::FONT_MONO;
            ImGui::InputStringMultiline("", procedure.meta.source, {-1, 0}, ImGuiInputTextFlags_NoHorizontalScroll);
        }
    }

    //--------------------------------------------------------//

    if (toDelete.has_value() == true)
    {
        SQASSERT(mWorkingAction->procedures.count(*toDelete) == 1u, "");
        mWorkingAction->procedures.erase(*toDelete);
        update_sorted_procedures();
    }

    if (toRename.has_value() == true)
    {
        SQASSERT(mWorkingAction->procedures.count(toRename->first) == 1u, "");
        if (mWorkingAction->procedures.count(toRename->second) == 0u)
        {
            auto node = mWorkingAction->procedures.extract(toRename->first);
            node.key() = toRename->second;
            mWorkingAction->procedures.insert(std::move(node));
            update_sorted_procedures();
        }
    }

    if (toCopy.has_value() == true)
    {
        SQASSERT(mWorkingAction->procedures.count(toCopy->first) == 1u, "");
        if (mWorkingAction->procedures.count(toCopy->second) == 0u)
        {
            auto& sourceRef = mWorkingAction->procedures.at(toCopy->first);
            mWorkingAction->procedures.emplace(toCopy->second, sourceRef);
            update_sorted_procedures();
        }
    }
}

//============================================================================//

void ActionEditor::impl_show_widget_timeline()
{
    if (mWorkingAction == nullptr) return;
    ImGui::SetNextWindowSizeConstraints(MIN_SIZE_TIMELINE, MAX_SIZE_TIMELINE);
    const ImGui::ScopeWindow window = { "Edit Timeline", 0 };
    if (window.open == false) return;

    //--------------------------------------------------------//

    const size_t workingActionLength = mWorkingAction->timeline.size();

    const uint16_t workingActionFrame = mReferenceAction->mCurrentFrame;

    //--------------------------------------------------------//

    Vector<Tuple<Action::Procedure&, uint16_t>> framesToToggle;
    Vector<Tuple<Action::Procedure&, uint16_t>> framesToInsert;

    //--------------------------------------------------------//

    const ImGui::ScopeFont font = ImGui::FONT_MONO;
    const ImGui::ScopeStyle_ButtonTextAlign align = {0, 0};

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 150.f);

    for (auto& [key, procedure] : mSortedProcedures)
    {
        const ImGui::ScopeID idScope = key.c_str();

        ImGui::Text("%s: "_fmt_(key));

        ImGui::NextColumn();
        ImGui::Spacing();

        for (uint16_t i = 0u; i < workingActionLength; ++i)
        {
            ImGui::SameLine();

            const bool selected = algo::exists(procedure.meta.frames, i);

            const String tooltip = "Frame: %d"_fmt_(i);
            const String hiddenLabel = "##%d"_fmt_(i);

            if (ImGui::Selectable(hiddenLabel.c_str(), selected, 0, {20u, 20u}))
            {
                framesToToggle.emplace_back(procedure, i);
            }
            ImGui::HoverTooltip(tooltip.c_str());

            const ImVec2 rectMin = ImGui::GetItemRectMin();
            const ImVec2 rectMax = ImGui::GetItemRectMax();

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            if (selected == false) drawList->AddRectFilled(rectMin, rectMax, ImGui::GetColorU32(ImGuiCol_FrameBg));
            drawList->AddRect(rectMin, rectMax, ImGui::GetColorU32(ImGuiCol_Border)); // inefficient, lots of overlap
        }

        ImGui::NextColumn();
        ImGui::Separator();
    }

    ImGui::NextColumn();
    ImGui::Spacing();

    for (uint16_t i = 0u; i < workingActionLength; ++i)
    {
        ImGui::SameLine();

        const String label = "%d"_fmt_(i);

        const bool open = ImGui::IsPopupOpen(label.c_str());
        if (ImGui::Selectable(label.c_str(), open, 0, {20u, 20u}))
            ImGui::OpenPopup(label.c_str());

        if (workingActionFrame != i)
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGui::MOUSE_RIGHT))
                scrub_to_frame(i);

        if (ImGui::BeginPopup(label.c_str()))
        {
            if (ImGui::MenuItem("insert frame before"))
            {
                for (auto& [key, procedure] : mWorkingAction->procedures)
                    framesToInsert.emplace_back(procedure, i);
            }
            if (ImGui::MenuItem("insert frame after"))
            {
                for (auto& [key, procedure] : mWorkingAction->procedures)
                    framesToInsert.emplace_back(procedure, i + 1u);
            }

            ImGui::EndPopup();
        }

        const ImVec2 rectMin = ImGui::GetItemRectMin();
        const ImVec2 rectMax = ImGui::GetItemRectMax();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRect(rectMin, rectMax, ImGui::GetColorU32(ImGuiCol_Border)); // inefficient, lots of overlap

        if (i == workingActionFrame)
        {
            const ImVec2 scrubberTop = { rectMin.x, rectMax.y + 2.f };
            const ImVec2 scrubberLeft = { rectMin.x - 3.f, scrubberTop.y + 8.f };
            const ImVec2 scrubberRight = { rectMin.x + 3.f, scrubberTop.y + 8.f };
            drawList->AddTriangle(scrubberTop, scrubberLeft, scrubberRight, ImGui::GetColorU32(ImGuiCol_Border));
        }
    }

    ImGui::Columns();

    //--------------------------------------------------------//

    for (auto& [procedure, when] : framesToToggle)
    {
        const auto predicate = [when=when](uint16_t frame) { return frame >= when; };
        const auto iter = algo::find_if(procedure.meta.frames, predicate);

        // frame is after all existing frames
        if (iter == procedure.meta.frames.end()) procedure.meta.frames.push_back(when);

        // frame is already enabled
        else if (*iter == when) procedure.meta.frames.erase(iter);

        // insert the frame before the iterator
        else procedure.meta.frames.insert(iter, when);
    }

    for (auto& [procedure, when] : framesToInsert)
    {
        for (uint16_t& frame : procedure.meta.frames)
            if (frame >= when) ++frame;
    }

    //--------------------------------------------------------//

    if (framesToToggle.empty() == false || framesToInsert.empty() == false)
    {
        mWorkingAction->rebuild_timeline();
        //update_sorted_procedures();
    }
}

//============================================================================//

bool ActionEditor::build_working_procedures()
{
    SQASSERT(mWorkingAction != nullptr, "no working action");

    ActionBuilder& actionBuilder = mFightWorld->get_action_builder();

    bool success = true;

    for (auto& [key, procedure] : mWorkingAction->procedures)
    {
        procedure.commands = actionBuilder.build_procedure(*mWorkingAction, procedure.meta.source);
        success &= procedure.commands.empty() == false;
    }

    actionBuilder.flush_logged_errors("errors building working action:");

    return success;
}

//============================================================================//

void ActionEditor::apply_working_changes()
{
    SQASSERT(mWorkingAction && mReferenceAction, "no working action");

    if (build_working_procedures() == false)
    {
//        sq::log_info("Building procedures failed, changes not applied.");
//        return;
    }

    const uint16_t restoreFrame = mReferenceAction->mCurrentFrame;

    mReferenceAction->blobs.clear();
    mReferenceAction->emitters.clear();
    mReferenceAction->procedures.clear();

    for (const auto& [key, ptr] : mWorkingAction->blobs)
        mReferenceAction->blobs.emplace(key, *ptr);

    for (const auto& [key, ptr] : mWorkingAction->emitters)
        mReferenceAction->emitters.emplace(key, *ptr);

    for (const auto& [key, procedure] : mWorkingAction->procedures)
        mReferenceAction->procedures.emplace(key, procedure);

    mReferenceAction->rebuild_timeline();

    if (algo::exists(mChangedActions, mWorkingAction->type) == false)
        mChangedActions.push_back(mWorkingAction->type);

    scrub_to_frame(restoreFrame);

    SQASSERT(mWorkingAction->has_changes(*mReferenceAction) == false, "");
}

//============================================================================//

void ActionEditor::revert_working_changes()
{
    SQASSERT(mWorkingAction && mReferenceAction, "no working action");

    mWorkingAction->blobs.clear();
    mWorkingAction->emitters.clear();
    mWorkingAction->procedures.clear();

    for (const auto& [key, ptr] : mReferenceAction->blobs)
        mWorkingAction->blobs.emplace(key, *ptr);

    for (const auto& [key, ptr] : mReferenceAction->emitters)
        mWorkingAction->emitters.emplace(key, *ptr);

    for (const auto& [key, procedure] : mReferenceAction->procedures)
        mWorkingAction->procedures.emplace(key, procedure);

    mWorkingAction->rebuild_timeline();
    update_sorted_procedures();

    SQASSERT(mWorkingAction->has_changes(*mReferenceAction) == false, "");
}

//============================================================================//

void ActionEditor::update_sorted_procedures()
{
    SQASSERT(mWorkingAction != nullptr, "no working action");

    mSortedProcedures.clear();
    mSortedProcedures.reserve(mWorkingAction->procedures.size());

    std::transform(mWorkingAction->procedures.begin(), mWorkingAction->procedures.end(),
                   std::back_inserter(mSortedProcedures), [](auto& ref) { return &ref; });

    if (mSortProceduresEnabled == true)
    {
        const auto compare = [](ProcedurePair* lhs, ProcedurePair* rhs) -> bool
        {
            if (lhs->second.meta.frames < rhs->second.meta.frames)
                return true;
            if (lhs->second.meta.frames == rhs->second.meta.frames)
                return lhs->first < rhs->first;
            return false;
        };

        std::sort(mSortedProcedures.begin(), mSortedProcedures.end(), compare);
    }
}

//============================================================================//

void ActionEditor::save_changed_actions()
{
    while (mChangedActions.empty() == false)
    {
        const Action* const action = fighter->get_action(mChangedActions.back());
        SQASSERT(action != nullptr, "you can't change none :P");

        const JsonValue json = mFightWorld->get_action_builder().serialise_as_json(*action);

        sq::save_string_to_file(action->path, json.dump(2));

        mChangedActions.pop_back();
    }
}

//============================================================================//

void ActionEditor::scrub_to_frame(uint16_t frame)
{
    SQASSERT(mWorkingAction != nullptr, "no working action");

    privateFighter->switch_action(ActionType::None);

    ParticleEmitter::reset_random_seed(mRandomSeed);
    mFightWorld->get_particle_system().clear();

    privateFighter->switch_action(mWorkingAction->type);

    for (uint16_t i = 0u; i < frame; ++i)
        mFightWorld->tick();
}

//============================================================================//

void ActionEditor::handle_message(const message::fighter_action_finished& /*msg*/)
{
    if (mWorkingAction != nullptr)
    {
        ParticleEmitter::reset_random_seed(mRandomSeed);
        privateFighter->switch_action(mWorkingAction->type);
    }
}
