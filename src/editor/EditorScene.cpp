#include "editor/EditorScene.hpp"

#include "main/DebugGui.hpp"
#include "main/Options.hpp"
#include "main/SmashApp.hpp"

#include "game/Action.hpp"
#include "game/Emitter.hpp"
#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"
#include "game/ParticleSystem.hpp"
#include "game/Stage.hpp"

#include "render/DebugRender.hpp"
#include "render/RenderObject.hpp"
#include "render/Renderer.hpp"

#include "editor/EditorCamera.hpp"

#include "stages/TestZone_Stage.hpp"
#include "fighters/Sara_Fighter.hpp"
#include "fighters/Tux_Fighter.hpp"
#include "fighters/Mario_Fighter.hpp"

#include "stages/TestZone_Render.hpp"
#include "fighters/Sara_Render.hpp"
#include "fighters/Tux_Render.hpp"
#include "fighters/Mario_Render.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/gl/Context.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Armature.hpp>

using namespace sts;

//============================================================================//

constexpr const float WIDTH_EDITORS   = 520.f;
constexpr const float HEIGHT_TIMELINE = 80.f;
constexpr const float WIDTH_NAVIGATOR = 240.f;

//============================================================================//

EditorScene::EditorScene(SmashApp& smashApp)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    auto& options = mSmashApp.get_options();

    options.render_hit_blobs = true;
    options.render_hurt_blobs = true;
    options.render_diamonds = true;
    options.render_skeletons = true;

    options.editor_mode = true;

    auto& window = mSmashApp.get_window();

    window.set_key_repeat(true);
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
                {
                    if (event.data.keyboard.shift) scrub_to_frame_previous(*mActiveActionContext);
                    else tick_action_context(*mActiveActionContext);
                }
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

    if (mActiveHurtblobsContext != nullptr)
    {
        apply_working_changes(*mActiveHurtblobsContext);
    }

    if (mActiveActionContext != nullptr)
    {
        apply_working_changes(*mActiveActionContext);

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

void EditorScene::render(double /*elapsed*/)
{
    // need to clear the window if we don't have an active context
    if (mActiveContext == nullptr)
    {
        auto& options = mSmashApp.get_options();
        auto& context = sq::Context::get();

        context.set_ViewPort(options.window_size);

        context.clear_Colour({0.f, 0.f, 0.f, 1.f});
        context.clear_Depth_Stencil();

        return; // EARLY RETURN
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

    const float blend = mPreviewMode == PreviewMode::Pause ? mBlendValue : float(mAccumulation / mTickTime);

    ctx.renderer->render_objects(blend);

    ctx.renderer->resolve_multisample();

    ctx.renderer->render_particles(ctx.world->get_particle_system(), blend);

    auto& options = mSmashApp.get_options();
    auto& debugRenderer = ctx.renderer->get_debug_renderer();

    if (options.render_hit_blobs == true)
        debugRenderer.render_hit_blobs(ctx.world->get_hit_blobs());

    if (options.render_hurt_blobs == true)
        debugRenderer.render_hurt_blobs(ctx.world->get_hurt_blobs());

    if (options.render_diamonds == true)
        for (const auto fighter : ctx.world->get_fighters())
            debugRenderer.render_diamond(*fighter);

    if (options.render_skeletons == true)
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
        mDoResetDockSounds = true;
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
            if (ImPlus::MenuItem("Reset Layout"))
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
            auto& options = mSmashApp.get_options();
            ImPlus::Checkbox("hit blobs", &options.render_hit_blobs);
            ImPlus::Checkbox("hurt blobs", &options.render_hurt_blobs);
            ImPlus::Checkbox("diamonds", &options.render_diamonds);
            ImPlus::Checkbox("skeletons", &options.render_skeletons);
        });

        //--------------------------------------------------------//

        ImPlus::if_Menu("Action", mActiveActionContext != nullptr, [&]()
        {
            if (ImPlus::MenuItem("Save", "Ctrl+S", false, mActiveActionContext->modified))
                save_changes(*mActiveActionContext);
            ImPlus::HoverTooltip("Save changes to the active action");

            Fighter& fighter = *mActiveActionContext->fighter;

            if (ImPlus::MenuItem("Reload Animations", nullptr, false, true))
                fighter.initialise_armature(sq::build_path("assets/fighters", sq::enum_to_string(fighter.type)));
            ImPlus::HoverTooltip("Reload animations for the active action");
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
                const auto numTotal = sq::enum_count_v<ActionType>;

                if (!ImPlus::CollapsingHeader("{} ({}/{})###{}"_format(fighterName, numLoaded, numTotal, fighterName)))
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
                        mSmashApp.get_window().set_window_title("SuperTuxSmash - Action Editor - {}/{}"_format(fighterName, actionName));

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
                    mSmashApp.get_window().set_window_title("SuperTuxSmash - Action Editor - {} HurtBlobs"_format(key));

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

void EditorScene::show_imgui_widgets()
{
    impl_show_widget_toolbar();
    impl_show_widget_navigator();
    impl_show_widget_hitblobs();
    impl_show_widget_emitters();
    impl_show_widget_sounds();
    impl_show_widget_script();
    impl_show_widget_hurtblobs();
    impl_show_widget_timeline();
    impl_show_widget_fighter();
}

//============================================================================//

void EditorScene::create_base_context(FighterEnum fighterKey, BaseContext& ctx)
{
    ctx.world = std::make_unique<FightWorld>(mSmashApp.get_options(), mSmashApp.get_audio_context());
    ctx.renderer = std::make_unique<Renderer>(mSmashApp.get_options());

    ctx.renderer->set_camera(std::make_unique<EditorCamera>(*ctx.renderer));

    auto stage = std::make_unique<TestZone_Stage>(*ctx.world);
    auto renderStage = std::make_unique<TestZone_Render>(*ctx.renderer, static_cast<TestZone_Stage&>(*stage));

    std::unique_ptr<Fighter> fighter;
    std::unique_ptr<RenderObject> renderFighter;

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

    const Fighter::Animations& anims = ctx.fighter->mAnimations;

    SWITCH (key.action) {

    CASE (NeutralFirst) ctx.timelineLength = anims.NeutralFirst.anim.totalTime;

    CASE (DashAttack)   ctx.timelineLength = anims.DashAttack.anim.totalTime;

    CASE (TiltDown)     ctx.timelineLength = anims.TiltDown.anim.totalTime;
    CASE (TiltForward)  ctx.timelineLength = anims.TiltForward.anim.totalTime;
    CASE (TiltUp)       ctx.timelineLength = anims.TiltUp.anim.totalTime;

    CASE (EvadeBack)    ctx.timelineLength = anims.EvadeBack.anim.totalTime;
    CASE (EvadeForward) ctx.timelineLength = anims.EvadeForward.anim.totalTime;
    CASE (Dodge)        ctx.timelineLength = anims.Dodge.anim.totalTime;

    CASE (ProneAttack)  ctx.timelineLength = anims.ProneAttack.anim.totalTime;
    CASE (ProneBack)    ctx.timelineLength = anims.ProneBack.anim.totalTime;
    CASE (ProneForward) ctx.timelineLength = anims.ProneForward.anim.totalTime;;
    CASE (ProneStand)   ctx.timelineLength = anims.ProneStand.anim.totalTime;;

    CASE (LedgeClimb) ctx.timelineLength = anims.LedgeClimb.anim.totalTime;

    CASE (SmashDown)    ctx.timelineLength = anims.SmashDownAttack.anim.totalTime;
    CASE (SmashForward) ctx.timelineLength = anims.SmashForwardAttack.anim.totalTime;
    CASE (SmashUp)      ctx.timelineLength = anims.SmashUpAttack.anim.totalTime;

    CASE (AirBack)    ctx.timelineLength = anims.AirBack.anim.totalTime;
    CASE (AirDown)    ctx.timelineLength = anims.AirDown.anim.totalTime;
    CASE (AirForward) ctx.timelineLength = anims.AirForward.anim.totalTime;
    CASE (AirNeutral) ctx.timelineLength = anims.AirNeutral.anim.totalTime;
    CASE (AirUp)      ctx.timelineLength = anims.AirUp.anim.totalTime;
    CASE (AirDodge)   ctx.timelineLength = anims.AirDodge.anim.totalTime;

    CASE (LandLight)  ctx.timelineLength = anims.LandLight.anim.totalTime;
    CASE (LandHeavy)  ctx.timelineLength = anims.LandHeavy.anim.totalTime;
    CASE (LandTumble) ctx.timelineLength = anims.LandTumble.anim.totalTime;

    CASE (LandAirBack)    ctx.timelineLength = anims.LandAirBack.anim.totalTime;
    CASE (LandAirDown)    ctx.timelineLength = anims.LandAirDown.anim.totalTime;
    CASE (LandAirForward) ctx.timelineLength = anims.LandAirForward.anim.totalTime;
    CASE (LandAirNeutral) ctx.timelineLength = anims.LandAirNeutral.anim.totalTime;
    CASE (LandAirUp)      ctx.timelineLength = anims.LandAirUp.anim.totalTime;

    CASE (SpecialDown)    ctx.timelineLength = 32u;
    CASE (SpecialForward) ctx.timelineLength = 32u;
    CASE (SpecialNeutral) ctx.timelineLength = 32u;
    CASE (SpecialUp)      ctx.timelineLength = 32u;

    CASE (HopBack, JumpBack)       ctx.timelineLength = anims.JumpBack.anim.totalTime;
    CASE (HopForward, JumpForward) ctx.timelineLength = anims.JumpForward.anim.totalTime;
    CASE (AirHopBack)              ctx.timelineLength = anims.AirHopBack.anim.totalTime;
    CASE (AirHopForward)           ctx.timelineLength = anims.AirHopForward.anim.totalTime;

    CASE (DashStart) ctx.timelineLength = anims.DashStart.anim.totalTime;
    CASE (DashBrake) ctx.timelineLength = anims.Brake.anim.totalTime;

    CASE (None) SQASSERT(false, "can't edit nothing");

    } SWITCH_END;

    ctx.timelineLength += 1u;

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

    ctx.fighter->state_transition(Fighter::State::EditorPreview, 0u, nullptr, 0u, nullptr);
    ctx.world->tick();

    ctx.savedData = std::make_unique<decltype(Fighter::mHurtBlobs)>(ctx.fighter->mHurtBlobs);
    ctx.undoStack.push_back(std::make_unique<decltype(Fighter::mHurtBlobs)>(ctx.fighter->mHurtBlobs));

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
        ctx.world->editor_clear_hitblobs();

        // always reload script so that error message updates
        action.load_wren_from_string();

        scrub_to_frame_current(ctx);

        ctx.undoStack.erase(ctx.undoStack.begin() + ++ctx.undoIndex, ctx.undoStack.end());
        ctx.undoStack.emplace_back(action.clone());

        ctx.modified = action.has_changes(*ctx.savedData);
    }
}

void EditorScene::apply_working_changes(HurtblobsContext& ctx)
{
    if (ctx.fighter->mHurtBlobs != *ctx.undoStack[ctx.undoIndex])
    {
        ctx.world->editor_clear_hurtblobs();
        for (auto& [key, blob] : ctx.fighter->mHurtBlobs)
            ctx.world->enable_hurtblob(&blob);

        ctx.world->tick();

        ctx.undoStack.erase(ctx.undoStack.begin() + ++ctx.undoIndex, ctx.undoStack.end());
        ctx.undoStack.push_back(std::make_unique<decltype(Fighter::mHurtBlobs)>(ctx.fighter->mHurtBlobs));

        ctx.modified = ctx.fighter->mHurtBlobs != *ctx.savedData;
    }
}

//----------------------------------------------------------------------------//

void EditorScene::do_undo_redo(ActionContext& ctx, bool redo)
{
    const size_t oldIndex = ctx.undoIndex;

    if (!redo && ctx.undoIndex > 0u) --ctx.undoIndex;
    if (redo && ctx.undoIndex < ctx.undoStack.size() - 1u) ++ctx.undoIndex;

    if (ctx.undoIndex != oldIndex)
    {
        ctx.world->editor_clear_hitblobs();

        Action& action = *ctx.fighter->get_action(ctx.key.action);
        action.apply_changes(*ctx.undoStack[ctx.undoIndex]);

        scrub_to_frame_current(ctx);

        ctx.modified = action.has_changes(*ctx.savedData);
    }
}

void EditorScene::do_undo_redo(HurtblobsContext& ctx, bool redo)
{
    const size_t oldIndex = ctx.undoIndex;

    if (!redo && ctx.undoIndex > 0u) --ctx.undoIndex;
    if (redo && ctx.undoIndex < ctx.undoStack.size() - 1u) ++ctx.undoIndex;

    if (ctx.undoIndex != oldIndex)
    {
        ctx.fighter->mHurtBlobs = *ctx.undoStack[ctx.undoIndex];

        ctx.world->editor_clear_hurtblobs();
        for (auto& [key, blob] : ctx.fighter->mHurtBlobs)
            ctx.world->enable_hurtblob(&blob);

        ctx.world->tick();

        ctx.modified = ctx.fighter->mHurtBlobs != *ctx.savedData;
    }
}

//----------------------------------------------------------------------------//

void EditorScene::save_changes(ActionContext& ctx)
{
    const Action& action = *ctx.fighter->get_action(ctx.key.action);

    JsonValue json;

    auto& blobs = json["blobs"] = JsonValue::object();
    auto& emitters = json["emitters"] = JsonValue::object();
    auto& sounds = json["sounds"] = JsonValue::object();

    for (const auto& [key, blob] : action.mBlobs)
        blob.to_json(blobs[key.c_str()]);

    for (const auto& [key, emitter] : action.mEmitters)
        emitter.to_json(emitters[key.c_str()]);

    for (const auto& [key, sound] : action.mSounds)
        sound.to_json(sounds[key.c_str()]);

    sq::save_string_to_file(action.path + ".json", json.dump(2));
    sq::save_string_to_file(action.path + ".wren", action.mWrenSource);

    ctx.savedData = action.clone();
    ctx.modified = false;
}

void EditorScene::save_changes(HurtblobsContext& ctx)
{
    JsonValue json;
    for (const auto& [key, blob] : ctx.fighter->mHurtBlobs)
        blob.to_json(json[key.c_str()]);

    sq::save_string_to_file("assets/fighters/{}/HurtBlobs.json"_format(ctx.fighter->type), json.dump(2));

    *ctx.savedData = ctx.fighter->mHurtBlobs;
    ctx.modified = false;
}

//============================================================================//

void EditorScene::scrub_to_frame(ActionContext& ctx, int frame)
{
    // reset the world and fighter

    ctx.world->get_particle_system().reset_random_seed(mRandomSeed);
    ctx.world->get_particle_system().clear();

    if (ctx.fighter->mActiveAction != nullptr)
    {
        ctx.fighter->mActiveAction->do_cancel();
        ctx.fighter->mActiveAction = nullptr;
    }

    ctx.fighter->previous = ctx.fighter->current = Fighter::InterpolationData();
    ctx.fighter->current.pose = ctx.fighter->mArmature.get_rest_pose();
    ctx.fighter->status = Fighter::Status();
    ctx.fighter->mForceSwitchAction = ActionType::None;
    ctx.fighter->mTranslate = Vec2F();

    if (ctx.key.action == ActionType::AirBack || ctx.key.action == ActionType::AirDown || ctx.key.action == ActionType::AirForward ||
        ctx.key.action == ActionType::AirNeutral || ctx.key.action == ActionType::AirUp || ctx.key.action == ActionType::AirDodge)
    {
        ctx.fighter->status.position = Vec2F(0.f, 1.f);
        ctx.fighter->stats.gravity = 0.f;

        ctx.fighter->state_transition(FighterState::JumpFall, 0u, &ctx.fighter->mAnimations.FallingLoop, 0u, nullptr);
    }
    else if (ctx.key.action == ActionType::DashBrake)
    {
        ctx.fighter->status.velocity.x = ctx.fighter->stats.dash_speed + ctx.fighter->stats.traction;
        ctx.fighter->state_transition(FighterState::Neutral, 0u, &ctx.fighter->mAnimations.DashingLoop, 0u, nullptr);
    }
    else
        ctx.fighter->state_transition(FighterState::Neutral, 0u, &ctx.fighter->mAnimations.NeutralLoop, 0u, nullptr);

    // tick once to apply neutral/falling animation
    ctx.world->tick();

    // start the action on the next tick
    ctx.fighter->mForceSwitchAction = ctx.key.action;

    // todo: should be possible to charge some special actions, so need more generic test for this
    const bool chargeAction = ctx.key.action == ActionType::SmashDown ||
                              ctx.key.action == ActionType::SmashForward ||
                              ctx.key.action == ActionType::SmashUp;

    // finally, scrub to the desired frame
    for (ctx.currentFrame = chargeAction ? -2 : -1; ctx.currentFrame < frame;)
        tick_action_context(ctx);

    if (ctx.fighter->mActiveAction != nullptr && ctx.currentFrame >= 0)
    {
        const int actionFrame = int(ctx.fighter->mActiveAction->mCurrentFrame) - 1;
        SQASSERT(ctx.currentFrame == actionFrame, "out of sync: timeline = {}, action = {}"_format(ctx.currentFrame, actionFrame));
    }
}

void EditorScene::scrub_to_frame_current(ActionContext& ctx)
{
    // this function is called when some value is updated
    scrub_to_frame(ctx, ctx.currentFrame);
}

void EditorScene::tick_action_context(ActionContext& ctx)
{
    ctx.currentFrame += 1;

    if (ctx.currentFrame == ctx.timelineLength)
    {
        if (mIncrementSeed) ++mRandomSeed;
        scrub_to_frame(ctx, -1);
    }
    else ctx.world->tick();
}

void EditorScene::scrub_to_frame_previous(ActionContext& ctx)
{
    if (ctx.currentFrame == -1) scrub_to_frame(ctx, ctx.timelineLength - 1);
    else scrub_to_frame(ctx, ctx.currentFrame - 1);
}
