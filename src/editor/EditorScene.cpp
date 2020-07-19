#include "editor/EditorScene.hpp"

#include "stages/TestZone_Stage.hpp"
#include "fighters/Sara_Fighter.hpp"
#include "fighters/Tux_Fighter.hpp"
#include "fighters/Mario_Fighter.hpp"

#include "stages/TestZone_Render.hpp"
#include "fighters/Sara_Render.hpp"
#include "fighters/Tux_Render.hpp"
#include "fighters/Mario_Render.hpp"

#include "render/DebugRender.hpp"

#include "game/private/PrivateFighter.hpp"

#include "main/DebugGui.hpp"

#include <sqee/macros.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/gl/Context.hpp>

#include <sqee/misc/Algorithms.hpp>

namespace algo = sq::algo;
namespace maths = sq::maths;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

constexpr const float WIDTH_EDITORS   = 480;
constexpr const float HEIGHT_TIMELINE = 80;
constexpr const float WIDTH_NAVIGATOR = 240;

//============================================================================//

EditorScene::EditorScene(SmashApp& smashApp)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    widget_toolbar.func = [this]() { impl_show_widget_toolbar(); };
    widget_navigator.func = [this]() { impl_show_widget_navigator(); };
    widget_hitblobs.func = [this]() { impl_show_widget_hitblobs(); };
    widget_emitters.func = [this]() { impl_show_widget_emitters(); };
    widget_script.func = [this]() { impl_show_widget_script(); };
    widget_hurtblobs.func = [this]() { impl_show_widget_hurtblobs(); };
    widget_timeline.func = [this]() { impl_show_widget_timeline(); };
    widget_fighter.func = [this]() { impl_show_widget_fighter(); };

    //--------------------------------------------------------//

    smashApp.get_globals().renderHitBlobs = true;
    smashApp.get_globals().renderHurtBlobs = true;
    smashApp.get_globals().renderDiamonds = true;
    smashApp.get_globals().renderSkeletons = true;

    smashApp.get_globals().editorMode = true;

    smashApp.get_window().set_key_repeat(true);
}

EditorScene::~EditorScene() = default;

//============================================================================//

void EditorScene::handle_event(sq::Event event)
{
    if (event.type == sq::Event::Type::Keyboard_Press)
    {
        if (event.data.keyboard.key == sq::Keyboard_Key::Space)
        {
            if (mPreviewMode == PreviewMode::Pause)
            {
                if (mActiveActionContext != nullptr)
                    tick_action_context(*mActiveActionContext);
            }
            else mPreviewMode = PreviewMode::Pause;
        }

        if (event.data.keyboard.ctrl && event.data.keyboard.key == sq::Keyboard_Key::Z)
        {
            if (mActiveActionContext != nullptr)
                do_undo_redo(*mActiveActionContext, event.data.keyboard.shift);

            if (mActiveHurtblobsContext != nullptr)
                do_undo_redo(*mActiveHurtblobsContext, event.data.keyboard.shift);
        }
    }

    if (event.type == sq::Event::Type::Mouse_Scroll)
    {
        if (mActiveContext != nullptr)
        {
            auto& camera = static_cast<EditorCamera&>(mActiveContext->renderer->get_camera());
            camera.update_from_scroll(event.data.scroll.delta);
        }
    }
}

//============================================================================//

void EditorScene::refresh_options()
{
    if (mActiveContext != nullptr)
        mActiveContext->renderer->refresh_options();
}

//============================================================================//

void EditorScene::update()
{
    if (mActiveActionContext != nullptr)
        apply_working_changes(*mActiveActionContext);

    if (mActiveHurtblobsContext != nullptr)
        apply_working_changes(*mActiveHurtblobsContext);

    if (mActiveActionContext != nullptr)
    {
        if (mDoRestartAction == true)
        {
            scrub_to_frame(*mActiveActionContext, -1);
            mDoRestartAction = false;
        }

        if (mPreviewMode != PreviewMode::Pause)
            tick_action_context(*mActiveActionContext);
    }
}

//============================================================================//

void EditorScene::render(double elapsed)
{
    // only need to clear the window if we don't have an active context
    if (mActiveContext == nullptr)
    {
        auto& context = sq::Context::get();

        context.set_ViewPort(mSmashApp.get_options().Window_Size);

        context.clear_Colour({0.f, 0.f, 0.f, 1.f});
        context.clear_Depth_Stencil();

        return;
    }

    //--------------------------------------------------------//

    BaseContext& ctx = *mActiveContext;

    if (ImGui::GetIO().WantCaptureMouse == false)
    {
        const Vec2I position = mSmashApp.get_input_devices().get_cursor_location(true);
        const Vec2I windowSize = Vec2I(mSmashApp.get_window().get_window_size());

        if (position.x >= 0 && position.y >= 0 && position.x < windowSize.x && position.y < windowSize.y)
        {
            const bool leftPressed = mSmashApp.get_input_devices().is_pressed(sq::Mouse_Button::Left);
            const bool rightPressed = mSmashApp.get_input_devices().is_pressed(sq::Mouse_Button::Right);

            auto& camera = static_cast<EditorCamera&>(ctx.renderer->get_camera());

            camera.update_from_mouse(leftPressed, rightPressed, Vec2F(position));
        }
    }

    const float accum = float(elapsed);
    const float blend = mPreviewMode == PreviewMode::Pause ? mBlendValue : float(mAccumulation / mTickTime);

    ctx.renderer->render_objects(accum, blend);

    ctx.renderer->resolve_multisample();

    ctx.renderer->render_particles(ctx.world->get_particle_system(), accum, blend);

    auto& debugRenderer = ctx.renderer->get_debug_renderer();

    if (mSmashApp.get_globals().renderHitBlobs == true)
        debugRenderer.render_hit_blobs(ctx.world->get_hit_blobs());

    if (mSmashApp.get_globals().renderHurtBlobs == true)
        debugRenderer.render_hurt_blobs(ctx.world->get_hurt_blobs());

    if (mSmashApp.get_globals().renderDiamonds == true)
        for (const auto fighter : ctx.world->get_fighters())
            debugRenderer.render_diamond(*fighter);

    if (mSmashApp.get_globals().renderSkeletons == true)
        for (const auto fighter : ctx.world->get_fighters())
            debugRenderer.render_skeleton(*fighter);

    ctx.renderer->finish_rendering();
}

//============================================================================//

void EditorScene::impl_show_widget_toolbar()
{
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    const auto windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar |
                             ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    const ImPlus::ScopeWindow window = { "main", windowFlags };
    ImGui::PopStyleVar(2);

    mDockMainId = ImGui::GetID("MainDockSpace");

    if (mWantResetDocks == true)
    {
        mDoResetDockNavigator = true;
        mDoResetDockHitblobs = true;
        mDoResetDockEmitters = true;
        mDoResetDockScript = true;
        mDoResetDockHurtblobs = true;
        mDoResetDockTimeline = true;
        mDoResetDockFighter = true;

        const auto viewSize = ImGui::GetWindowViewport()->Size;

        ImGui::DockBuilderRemoveNode(mDockMainId);
        ImGui::DockBuilderAddNode(mDockMainId, 1 << 10);
        ImGui::DockBuilderSetNodeSize(mDockMainId, viewSize);

        ImGui::DockBuilderSplitNode(mDockMainId, ImGuiDir_Right, 0.2f, &mDockRightId, &mDockNotRightId);
        ImGui::DockBuilderSplitNode(mDockNotRightId, ImGuiDir_Down, 0.2f, &mDockDownId, &mDockNotDownId);
        ImGui::DockBuilderSplitNode(mDockNotDownId, ImGuiDir_Left, 0.2f, &mDockLeftId, &mDockNotLeftId);

        ImGui::DockBuilderSetNodeSize(mDockRightId, {WIDTH_EDITORS, viewSize.y});
        ImGui::DockBuilderSetNodeSize(mDockDownId, {viewSize.x, HEIGHT_TIMELINE});
        ImGui::DockBuilderSetNodeSize(mDockLeftId, {WIDTH_NAVIGATOR, viewSize.y});

        ImGui::DockBuilderFinish(mDockMainId);
    }
    mWantResetDocks = false;

    ImGui::DockSpace(mDockMainId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode);

    if (window.show == false) return;

    //--------------------------------------------------------//

    ImPlus::if_MenuBar([&]()
    {
        ImPlus::if_Menu("Menu", true, [&]()
        {
            if (ImGui::MenuItem("Reset Layout"))
                mWantResetDocks = true;
            ImPlus::HoverTooltip("Reset editor layout to default");

            if (ImPlus::MenuItem("Save All", "Ctrl+Shift+S"))
            {
                // todo: show a popup listing everything that has changed
                for (auto& [key, ctx] : mActionContexts)
                    if (ctx.modified)
                        save_changes(ctx);
            }
            ImPlus::HoverTooltip("Save changes to all modified actions");
        });

        //--------------------------------------------------------//

        ImPlus::if_Menu("Render", true, [&]()
        {
            Globals& globals = mSmashApp.get_globals();
            ImPlus::Checkbox("hit blobs", &globals.renderHitBlobs);
            ImPlus::Checkbox("hurt blobs", &globals.renderHurtBlobs);
            ImPlus::Checkbox("diamonds", &globals.renderDiamonds);
            ImPlus::Checkbox("skeletons", &globals.renderSkeletons);
        });

        //--------------------------------------------------------//

        ImPlus::if_Menu("Action", mActiveActionContext != nullptr, [&]()
        {
            if (ImGui::MenuItem("Save", "Ctrl+S", false, mActiveActionContext->modified))
                save_changes(*mActiveActionContext);
            ImPlus::HoverTooltip("Save changes to the active action");
        });

        //--------------------------------------------------------//

//        if (mEditorMode == EditorMode::Animation)
//        {
//            if (ImGui::BeginMenu("Animation"))
//            {
//                // nothing in here for now
//                ImGui::EndMenu();
//            }

//            ImGui::PushItemWidth(160.f);
//            if (ImGui::ComboEnum("##AnimationList", mPreviewAnimation.type, ImGuiComboFlags_HeightLargest))
//            {
//                auto transition = privateFighter->transitions.editor_preview;

//                if (mPreviewAnimation.type != AnimationType::None)
//                {
//                    mPreviewAnimation = *fighter->get_animation(mPreviewAnimation.type);
//                    if (mPreviewAnimation.anim.times.back() == 0u)
//                    {
//                        mPreviewAnimation.anim.poses.emplace_back(mPreviewAnimation.anim.poses.back());
//                        mPreviewAnimation.anim.poses.emplace_back(mPreviewAnimation.anim.poses.front());

//                        mPreviewAnimation.anim.times.back() = 24u;
//                        mPreviewAnimation.anim.times.emplace_back(0u);
//                        mPreviewAnimation.anim.times.emplace_back(24u);

//                        mPreviewAnimation.anim.poseCount += 2u;
//                        mPreviewAnimation.anim.totalTime += 48u;
//                    }
//                    transition.animation = &mPreviewAnimation;
//                }

//                privateFighter->state_transition(transition);
//            }
//            ImGui::PopItemWidth();
//        }

        //--------------------------------------------------------//

        ImGui::Separator();

        ImGui::SetNextItemWidth(50.f);
        ImPlus::DragValue("##blend", mBlendValue, 0.f, 1.f, 0.01f, "%.2f");
        ImPlus::HoverTooltip("blend value to use when paused");

        if (ImPlus::RadioButton("Pause", mPreviewMode, PreviewMode::Pause)) {}
        ImPlus::HoverTooltip("press space to manually tick");
        if (ImPlus::RadioButton("Normal", mPreviewMode, PreviewMode::Normal)) mTickTime = 1.0 / 48.0;
        ImPlus::HoverTooltip("play preview at normal speed");
        if (ImPlus::RadioButton("Slow", mPreviewMode, PreviewMode::Slow)) mTickTime = 1.0 / 12.0;
        ImPlus::HoverTooltip("play preview at 1/4 speed");
        if (ImPlus::RadioButton("Slower", mPreviewMode, PreviewMode::Slower)) mTickTime = 1.0 / 3.0;
        ImPlus::HoverTooltip("play preview at 1/16 speed");

        ImGui::Separator();

        ImGui::SetNextItemWidth(120.f);
        if (ImPlus::InputValue("##seed", mRandomSeed, 1))
            if (mActiveActionContext != nullptr)
                scrub_to_frame_current(*mActiveActionContext);
        ImPlus::HoverTooltip("seed to use for random number generator");

        ImGui::Checkbox("RNG", &mIncrementSeed);
        ImPlus::HoverTooltip("increment when action repeats");
    });
}

//============================================================================//

void EditorScene::impl_show_widget_navigator()
{
    if (mDoResetDockNavigator) ImGui::SetNextWindowDockID(mDockLeftId);
    mDoResetDockNavigator = false;

    const ImPlus::ScopeWindow window = { "Navigator", 0 };
    if (window.show == false) return;

    //--------------------------------------------------------//

    const auto do_close_action = [this](ActionContext& ctx)
    {
        if (mActiveContext == &ctx)
        {
            mSmashApp.get_window().set_window_title("SuperTuxSmash - Action Editor");
            mActiveContext = mActiveActionContext = nullptr;
        }
        mActionContexts.erase(ctx.key);
    };

    const auto do_close_hurtblobs = [this](HurtblobsContext& ctx)
    {
        if (mActiveContext == &ctx)
        {
            mSmashApp.get_window().set_window_title("SuperTuxSmash - Action Editor");
            mActiveContext = mActiveHurtblobsContext = nullptr;
        }
        mHurtblobsContexts.erase(ctx.key);
    };

    //--------------------------------------------------------//

    ImPlus::if_TabBar("tabbar", ImGuiTabBarFlags_FittingPolicyScroll, [&]()
    {
        ImPlus::if_TabItemChild("Actions", 0, [&]()
        {
            for (int8_t i = 0; i < sq::enum_count_v<FighterEnum>; ++i)
            {
                const char* const fighterName = sq::to_c_string(FighterEnum(i));

                const auto numLoaded = std::count_if(mActionContexts.begin(), mActionContexts.end(),
                    [&](const auto& item) { return item.second.key.fighter == FighterEnum(i); });

                if (!ImPlus::CollapsingHeader("%s (%u/%u)###%s"_fmt_(fighterName, numLoaded, sq::enum_count_v<ActionType>, fighterName), 0))
                    continue;

                for (int8_t j = 0; j < sq::enum_count_v<ActionType>; ++j)
                {
                    const char* const actionName = sq::to_c_string(ActionType(j));
                    const ActionKey key = { FighterEnum(i), ActionType(j) };

                    const auto iter = mActionContexts.find(key);

                    const bool loaded = iter != mActionContexts.end();
                    const bool modified = loaded && iter->second.modified;
                    const bool active = loaded && &iter->second == mActiveContext;

                    const String label = sq::build_string(fighterName, '/', actionName, modified ? " *" : "");
                    const bool highlight = ImPlus::IsPopupOpen(label);

                    if (loaded) ImPlus::PushFont(ImPlus::FONT_BOLD);
                    if (ImPlus::Selectable(label, active || highlight) && !active)
                    {
                        mSmashApp.get_window().set_window_title("SuperTuxSmash - Action Editor - " + label);

                        mActiveHurtblobsContext = nullptr;
                        mActiveContext = mActiveActionContext = &get_action_context(key);
                        mActiveContext->renderer->refresh_options();
                    }
                    if (loaded) ImGui::PopFont();

                    ImPlus::if_PopupContextItem(nullptr, ImPlus::MOUSE_RIGHT, [&]()
                    {
                        if (ImGui::MenuItem("Close", nullptr, false, loaded))
                        {
                            if (modified) mConfirmCloseActionCtx = &iter->second;
                            else do_close_action(iter->second);
                        }
                        if (ImGui::MenuItem("Save", nullptr, false, modified))
                            save_changes(iter->second);
                    });
                }
            }
        });

        ImPlus::if_TabItemChild("HurtBlobs", 0, [&]()
        {
            for (int8_t i = 0; i < sq::enum_count_v<FighterEnum>; ++i)
            {
                const char* const fighterName = sq::to_c_string(FighterEnum(i));
                const FighterEnum key = FighterEnum(i);

                const auto iter = mHurtblobsContexts.find(key);

                const bool loaded = iter != mHurtblobsContexts.end();
                const bool modified = loaded && iter->second.modified;
                const bool active = loaded && &iter->second == mActiveContext;

                const String label = sq::build_string(fighterName, modified ? " *" : "");
                const bool highlight = ImPlus::IsPopupOpen(label);

                if (loaded) ImPlus::PushFont(ImPlus::FONT_BOLD);
                if (ImPlus::Selectable(label, active || highlight) && !active)
                {
                    mSmashApp.get_window().set_window_title("SuperTuxSmash - Action Editor - %s HurtBlobs"_fmt_(key));

                    mActiveActionContext = nullptr;
                    mActiveContext = mActiveHurtblobsContext = &get_hurtblobs_context(key);
                    mActiveContext->renderer->refresh_options();
                }
                if (loaded) ImGui::PopFont();

                ImPlus::if_PopupContextItem(nullptr, ImPlus::MOUSE_RIGHT, [&]()
                {
                    if (ImGui::MenuItem("Close", nullptr, false, loaded))
                    {
                        if (modified) mConfirmCloseHurtblobsCtx = &iter->second;
                        else do_close_hurtblobs(iter->second);
                    }
                    if (ImGui::MenuItem("Save", nullptr, false, modified))
                        save_changes(iter->second);
                });
            }
        });
    });


    //--------------------------------------------------------//

    if (mConfirmCloseActionCtx != nullptr)
    {
        const auto result = ImPlus::DialogConfirmation("Discard Changes", "Action has been modified, really discard changes?");
        if (result == ImPlus::DialogResult::Confirm) do_close_action(*mConfirmCloseActionCtx);
        if (result != ImPlus::DialogResult::None) mConfirmCloseActionCtx = nullptr;
    }

    if (mConfirmCloseHurtblobsCtx != nullptr)
    {
        const auto result = ImPlus::DialogConfirmation("Discard Changes", "HurtBlobs have been modified, really discard changes?");
        if (result == ImPlus::DialogResult::Confirm) do_close_hurtblobs(*mConfirmCloseHurtblobsCtx);
        if (result != ImPlus::DialogResult::None) mConfirmCloseHurtblobsCtx = nullptr;
    }
}

//============================================================================//1

void EditorScene::impl_show_widget_fighter()
{
    if (mActiveContext == nullptr) return;

    BaseContext& ctx = *mActiveContext;
    Fighter& fighter = *ctx.fighter;

    if (mDoResetDockFighter) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockFighter = false;

    const ImPlus::ScopeWindow window = { "Fighter", 0 };
    if (window.show == false) return;

    DebugGui::show_widget_fighter(fighter);
}

//============================================================================//

void EditorScene::create_base_context(FighterEnum fighterKey, BaseContext& ctx)
{
    ctx.world = std::make_unique<FightWorld>(mSmashApp.get_globals());
    ctx.renderer = std::make_unique<Renderer>(mSmashApp.get_globals(), mSmashApp.get_options());

    auto stage = std::make_unique<TestZone_Stage>(*ctx.world);
    auto renderStage = std::make_unique<TestZone_Render>(*ctx.renderer, static_cast<TestZone_Stage&>(*stage));

    UniquePtr<Fighter> fighter;
    UniquePtr<RenderObject> renderFighter;

    SWITCH (fighterKey)
    {
        CASE (Sara)
        {
            fighter = std::make_unique<Sara_Fighter>(0, *ctx.world);
            renderFighter = std::make_unique<Sara_Render>(*ctx.renderer, static_cast<Sara_Fighter&>(*fighter));
        }

        CASE (Tux)
        {
            fighter = std::make_unique<Tux_Fighter>(0, *ctx.world);
            renderFighter = std::make_unique<Tux_Render>(*ctx.renderer, static_cast<Tux_Fighter&>(*fighter));
        }

        CASE (Mario)
        {
            fighter = std::make_unique<Mario_Fighter>(0, *ctx.world);
            renderFighter = std::make_unique<Mario_Render>(*ctx.renderer, static_cast<Mario_Fighter&>(*fighter));
        }

        CASE_DEFAULT SQASSERT(false, "bad fighter setup");
    }
    SWITCH_END;

    ctx.fighter = fighter.get();
    ctx.renderFighter = renderFighter.get();
    ctx.privateFighter = fighter->impl.get();

    ctx.world->set_stage(std::move(stage));
    ctx.renderer->add_object(std::move(renderStage));

    ctx.world->add_fighter(std::move(fighter));
    ctx.renderer->add_object(std::move(renderFighter));
}

//----------------------------------------------------------------------------//

EditorScene::ActionContext& EditorScene::get_action_context(ActionKey key)
{
    if (auto iter = mActionContexts.find(key); iter != mActionContexts.end())
        return iter->second;

    ActionContext& ctx = mActionContexts[key];

    create_base_context(key.fighter, ctx);

    ctx.key = key;

    scrub_to_frame_current(ctx);

    ctx.savedData = ctx.fighter->get_action(key.action)->clone();
    ctx.undoStack.push_back(ctx.fighter->get_action(key.action)->clone());

    const auto& anims = ctx.privateFighter->animations;

    SWITCH (key.action) {

    CASE (NeutralFirst) ctx.timelineLength =  anims.NeutralFirst.anim.totalTime;
    CASE (TiltDown)     ctx.timelineLength = anims.TiltDown.anim.totalTime;
    CASE (TiltForward)  ctx.timelineLength = anims.TiltForward.anim.totalTime;
    CASE (TiltUp)       ctx.timelineLength = anims.TiltUp.anim.totalTime;
    CASE (DashAttack)   ctx.timelineLength = anims.DashAttack.anim.totalTime;

    CASE (AirBack)    ctx.timelineLength = anims.AirBack.anim.totalTime;
    CASE (AirDown)    ctx.timelineLength = anims.AirDown.anim.totalTime;
    CASE (AirForward) ctx.timelineLength = anims.AirForward.anim.totalTime;
    CASE (AirNeutral) ctx.timelineLength = anims.AirNeutral.anim.totalTime;
    CASE (AirUp)      ctx.timelineLength = anims.AirUp.anim.totalTime;

    CASE (EvadeBack)    ctx.timelineLength = anims.EvadeBack.anim.totalTime;
    CASE (EvadeForward) ctx.timelineLength = anims.EvadeForward.anim.totalTime;
    CASE (Dodge)        ctx.timelineLength = anims.Dodge.anim.totalTime;

    CASE (AirDodge) ctx.timelineLength = anims.AirDodge.anim.totalTime;

    CASE (SmashDown)    ctx.timelineLength = anims.SmashDownStart.anim.totalTime
                                            + anims.SmashDownCharge.anim.totalTime
                                            + anims.SmashDownAttack.anim.totalTime;

    CASE (SmashForward) ctx.timelineLength = anims.SmashForwardStart.anim.totalTime
                                            + anims.SmashForwardCharge.anim.totalTime
                                            + anims.SmashForwardAttack.anim.totalTime;

    CASE (SmashUp)      ctx.timelineLength = anims.SmashUpStart.anim.totalTime
                                            + anims.SmashUpCharge.anim.totalTime
                                            + anims.SmashUpAttack.anim.totalTime;

    CASE (SpecialDown)    ctx.timelineLength = 32u;
    CASE (SpecialForward) ctx.timelineLength = 32u;
    CASE (SpecialNeutral) ctx.timelineLength = 32u;
    CASE (SpecialUp)      ctx.timelineLength = 32u;

    CASE (None) SQASSERT(false, "");

    } SWITCH_END;

    return ctx;
}

//----------------------------------------------------------------------------//

EditorScene::HurtblobsContext& EditorScene::get_hurtblobs_context(FighterEnum key)
{
    if (auto iter = mHurtblobsContexts.find(key); iter != mHurtblobsContexts.end())
        return iter->second;

    HurtblobsContext& ctx = mHurtblobsContexts[key];

    create_base_context(key, ctx);

    ctx.key = key;

    ctx.privateFighter->state_transition(Fighter::State::EditorPreview, 0u, nullptr, 0u, nullptr);
    ctx.world->tick();

    ctx.savedData = std::make_unique<decltype(Fighter::hurtBlobs)>(ctx.fighter->hurtBlobs);
    ctx.undoStack.push_back(std::make_unique<decltype(Fighter::hurtBlobs)>(ctx.fighter->hurtBlobs));

    return ctx;
}

//============================================================================//

// TODO: should merge similar edits so that dragging a slider doesn't generate 50+ undo entries
// probably need to make some "can_merge_edits" methods for editables and to store the time for each record

void EditorScene::apply_working_changes(ActionContext& ctx)
{
    Action& action = *ctx.fighter->get_action(ctx.key.action);

    if (action.has_changes(*ctx.undoStack[ctx.undoIndex]))
    {
        scrub_to_frame_current(ctx);

        ctx.undoStack.erase(ctx.undoStack.begin() + ++ctx.undoIndex, ctx.undoStack.end());
        ctx.undoStack.emplace_back(action.clone());

        ctx.modified = action.has_changes(*ctx.savedData);
    }
}

void EditorScene::apply_working_changes(HurtblobsContext& ctx)
{
    if (ctx.fighter->hurtBlobs != *ctx.undoStack[ctx.undoIndex])
    {
        ctx.world->editor_disable_all_hurtblobs();
        for (auto& [key, blob] : ctx.fighter->hurtBlobs)
            if (ctx.hiddenKeys.find(key) == ctx.hiddenKeys.end())
                ctx.world->enable_hurt_blob(&blob);

        ctx.world->tick();

        ctx.undoStack.erase(ctx.undoStack.begin() + ++ctx.undoIndex, ctx.undoStack.end());
        ctx.undoStack.push_back(std::make_unique<decltype(Fighter::hurtBlobs)>(ctx.fighter->hurtBlobs));

        ctx.modified = ctx.fighter->hurtBlobs != *ctx.savedData;
    }
}

//----------------------------------------------------------------------------//

void EditorScene::do_undo_redo(ActionContext &ctx, bool redo)
{
    const size_t oldIndex = ctx.undoIndex;

    if (!redo && ctx.undoIndex > 0u) --ctx.undoIndex;
    if (redo && ctx.undoIndex < ctx.undoStack.size() - 1u) ++ctx.undoIndex;

    if (ctx.undoIndex != oldIndex)
    {
        Action& action = *ctx.fighter->get_action(ctx.key.action);
        action.apply_changes(*ctx.undoStack[ctx.undoIndex]);

        scrub_to_frame_current(ctx);

        ctx.modified = action.has_changes(*ctx.savedData);
    }
}

void EditorScene::do_undo_redo(HurtblobsContext &ctx, bool redo)
{
    const size_t oldIndex = ctx.undoIndex;

    if (!redo && ctx.undoIndex > 0u) --ctx.undoIndex;
    if (redo && ctx.undoIndex < ctx.undoStack.size() - 1u) ++ctx.undoIndex;

    if (ctx.undoIndex != oldIndex)
    {
        ctx.fighter->hurtBlobs = *ctx.undoStack[ctx.undoIndex];

        ctx.world->editor_disable_all_hurtblobs();
        for (auto& [key, blob] : ctx.fighter->hurtBlobs)
            if (ctx.hiddenKeys.find(key) == ctx.hiddenKeys.end())
                ctx.world->enable_hurt_blob(&blob);

        ctx.world->tick();

        ctx.modified = ctx.fighter->hurtBlobs != *ctx.savedData;
    }
}

//----------------------------------------------------------------------------//

void EditorScene::save_changes(ActionContext& ctx)
{
    const Action& action = *ctx.fighter->get_action(ctx.key.action);

    JsonValue json;

    auto& blobs = json["blobs"] = JsonValue::object();
    auto& emitters = json["emitters"] = JsonValue::object();

    for (const auto& [key, blob] : action.mBlobs)
        blob.to_json(blobs[key]);

    for (const auto& [key, emitter] : action.mEmitters)
        emitter.to_json(emitters[key]);

    sq::save_string_to_file(action.path + ".json", json.dump(2));
    sq::save_string_to_file(action.path + ".lua", action.mLuaSource);

    ctx.savedData = action.clone();
    ctx.modified = false;
}

void EditorScene::save_changes(HurtblobsContext& ctx)
{
    JsonValue json;
    for (const auto& [key, blob] : ctx.fighter->hurtBlobs)
        blob.to_json(json[key]);

    sq::save_string_to_file("assets/fighters/%s/HurtBlobs.json"_fmt_(ctx.fighter->type), json.dump(2));

    *ctx.savedData = ctx.fighter->hurtBlobs;
    ctx.modified = false;
}

//============================================================================//

void EditorScene::scrub_to_frame(ActionContext& ctx, int frame)
{
    // reset the world and fighter

    ParticleEmitter::reset_random_seed(mRandomSeed);
    ctx.world->get_particle_system().clear();

    if (ctx.fighter->current.action != nullptr)
        ctx.privateFighter->switch_action(ActionType::None);

    ctx.privateFighter->state_transition(Fighter::State::Neutral, 0u, &ctx.privateFighter->animations.NeutralLoop, 0u, nullptr);

    ctx.privateFighter->current.position = Vec2F();
    ctx.privateFighter->current.rotation = QuatF();
    ctx.fighter->current.facing = +1;
    ctx.fighter->mVelocity = Vec2F();
    ctx.fighter->mTranslate = Vec2F();

    ctx.world->tick();

    // start the action and scrub to the frame

    ctx.privateFighter->switch_action(ctx.key.action);

    for (ctx.currentFrame = -1; ctx.currentFrame < frame;)
        tick_action_context(ctx);

    SQASSERT(ctx.fighter->current.action == nullptr ||
             int(ctx.fighter->current.action->mCurrentFrame)-1 == ctx.currentFrame,
             "out of sync: %d | %d"_fmt_(int(ctx.fighter->current.action->mCurrentFrame)-1, ctx.currentFrame));
}

void EditorScene::scrub_to_frame_current(ActionContext& ctx)
{
    Action& action = *ctx.fighter->get_action(ctx.key.action);
    scrub_to_frame(ctx, action.mCurrentFrame - 1u);
}

void EditorScene::tick_action_context(ActionContext& ctx)
{
    ctx.currentFrame += 1;
    ctx.world->tick();

    if (ctx.currentFrame == ctx.timelineLength)
        mDoRestartAction = true;
}
