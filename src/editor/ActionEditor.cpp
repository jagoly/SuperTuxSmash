#include "editor/ActionEditor.hpp"

#include "DebugGlobals.hpp"

#include "stages/TestZone_Stage.hpp"
#include "fighters/Sara_Fighter.hpp"
#include "fighters/Tux_Fighter.hpp"

#include "stages/TestZone_Render.hpp"
#include "fighters/Sara_Render.hpp"
#include "fighters/Tux_Render.hpp"

#include "game/ActionBuilder.hpp"
#include "game/private/PrivateFighter.hpp"

#include <sqee/macros.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Files.hpp>

#include <algorithm> // for lots of stuff

namespace algo = sq::algo;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

constexpr const Vec2F MIN_SIZE_HIT_BLOBS   = { 400, 200 };
constexpr const Vec2F MAX_SIZE_HIT_BLOBS   = { 600, 1000 };

constexpr const Vec2F MIN_SIZE_PROCEDURES  = { 400, 200 };
constexpr const Vec2F MAX_SIZE_PROCEDURES  = { 600, 1000 };

constexpr const Vec2F MIN_SIZE_TIMELINE    = { 800, 200 };
constexpr const Vec2F MAX_SIZE_TIMELINE    = { 1500, 800 };

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
    widget_hit_blobs.func = [this]() { impl_show_widget_hit_blobs(); };
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
    mRenderer->render_particles(mFightWorld->get_particle_system(), accum, blend);

    if (mRenderBlobsEnabled == true)
    {
        mRenderer->render_blobs(mFightWorld->get_hit_blobs());
        mRenderer->render_blobs(mFightWorld->get_hurt_blobs());
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
            if (ImGui::InputSqeeEnumCombo("##ActionList", actionType, ImGuiComboFlags_HeightLargest))
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

        ImGui::EndMainMenuBar();
    }

    fighter->debug_show_fighter_widget();
}

//============================================================================//

void ActionEditor::impl_show_widget_hit_blobs()
{
    if (mWorkingAction == nullptr) return;
    ImGui::SetNextWindowSizeConstraints(MIN_SIZE_HIT_BLOBS, MAX_SIZE_HIT_BLOBS);
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

        if (collapseAll) ImGui::SetNextTreeNodeOpen(false);
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
                if (!mWorkingAction->blobs.contains(newKey))
                {
                    toRename.emplace(key, newKey);
                    ImGui::CloseCurrentPopup();
                }
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
                if (!mWorkingAction->blobs.contains(newKey))
                {
                    toCopy.emplace(key, newKey);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::EndPopup();
        }

        //--------------------------------------------------------//

        if (sectionOpen == true)
        {
            ImGui::InputVector(" Origin", blob->origin, '4');
            ImGui::InputValue(" Radius", blob->radius, 0.01f, '4');
            ImGui::InputValue(" Group", blob->group, 1);
            ImGui::InputValue(" KnockAngle", blob->knockAngle, 0.01f, '4');
            ImGui::InputValue(" KnockBase", blob->knockBase, 0.01f, '4');
            ImGui::InputValue(" KnockScale", blob->knockScale, 0.01f, '4');
            ImGui::InputValue(" Damage", blob->damage, 0.01f, '4');
            ImGui::InputSqeeEnumCombo(" Flavour", blob->flavour, 0);
            ImGui::InputSqeeEnumCombo(" Priority", blob->priority, 0);
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
        SQASSERT(!mWorkingAction->blobs.contains(toRename->second), "");
        mWorkingAction->blobs.modify_key(toRename->first, toRename->second);
    }

    if (toCopy.has_value() == true)
    {
        SQASSERT(mWorkingAction->blobs.contains(toCopy->first), "");
        SQASSERT(!mWorkingAction->blobs.contains(toCopy->second), "");
        auto sourcePtr = mWorkingAction->blobs[toCopy->first];
        mWorkingAction->blobs.emplace(toCopy->second, *sourcePtr);
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

        if (collapseAll) ImGui::SetNextTreeNodeOpen(false);
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
                if (!mWorkingAction->procedures.count(newKey))
                {
                    toRename.emplace(key, newKey);
                    ImGui::CloseCurrentPopup();
                }
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
                if (!mWorkingAction->procedures.count(newKey))
                {
                    toCopy.emplace(key, newKey);
                    ImGui::CloseCurrentPopup();
                }
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
        SQASSERT(mWorkingAction->procedures.count(toRename->second) == 0u, "");
        auto node = mWorkingAction->procedures.extract(toRename->first);
        node.key() = toRename->second;
        mWorkingAction->procedures.insert(std::move(node));
        update_sorted_procedures();
    }

    if (toCopy.has_value() == true)
    {
        SQASSERT(mWorkingAction->procedures.count(toCopy->first) == 1u, "");
        SQASSERT(mWorkingAction->procedures.count(toCopy->second) == 0u, "");
        auto& sourceRef = mWorkingAction->procedures.at(toCopy->first);
        mWorkingAction->procedures.emplace(toCopy->second, sourceRef);
        update_sorted_procedures();
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
    privateFighter->switch_action(mWorkingAction->type);

    for (uint16_t i = 0u; i < frame; ++i)
    {
        mFightWorld->tick();
    }
}

//============================================================================//

void ActionEditor::handle_message(const message::fighter_action_finished& msg)
{
    if (mWorkingAction != nullptr)
    {
        privateFighter->switch_action(mWorkingAction->type);
    }
}
