#include "editor/EditorScene.hpp"

#include "main/DebugGui.hpp"
#include "main/Options.hpp"
#include "main/SmashApp.hpp"

#include "game/Controller.hpp"
#include "game/EffectSystem.hpp"
#include "game/Emitter.hpp"
#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"
#include "game/ParticleSystem.hpp"
#include "game/SoundEffect.hpp"
#include "game/Stage.hpp"
#include "game/VisualEffect.hpp"

#include "render/DebugRender.hpp"
#include "render/Renderer.hpp"

#include "editor/EditorCamera.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Armature.hpp>
#include <sqee/vk/Helpers.hpp>
#include <sqee/vk/VulkanContext.hpp>

using namespace sts;

//============================================================================//

constexpr const float WIDTH_EDITORS   = 540.f;
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

    window.set_title("SuperTuxSmash - Editor");
    window.set_key_repeat(true);

    mController = std::make_unique<Controller>(mSmashApp.get_input_devices(), "config/player1.json");

    //--------------------------------------------------------//

    const auto& ctx = sq::VulkanContext::get();

    mImageProcessSetLayout = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding(0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment)
    });

    mImageProcessPipelineLayout = ctx.create_pipeline_layout (
        mImageProcessSetLayout, vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0u, sizeof(Vec2F))
    );

    //--------------------------------------------------------//

    for (const auto& entry : sq::parse_json_from_file("assets/FighterAnimations.json"))
        mFighterInfoCommon.animations.emplace_back(entry[0].get_ref<const String&>());

    for (const auto& entry : sq::parse_json_from_file("assets/FighterActions.json"))
        mFighterInfoCommon.actions.emplace_back(entry.get_ref<const String&>());

    for (const auto& entry : sq::parse_json_from_file("assets/FighterStates.json"))
        mFighterInfoCommon.states.emplace_back(entry.get_ref<const String&>());

    for (int8_t i = 0; i < sq::enum_count_v<FighterEnum>; ++i)
    {
        FighterInfo& info = mFighterInfos[i];

        for (const auto& entry : sq::parse_json_from_file("assets/fighters/{}/Animations.json"_format(FighterEnum(i))))
            info.animations.emplace_back(entry[0].get_ref<const String&>());

        for (const auto& entry : sq::parse_json_from_file("assets/fighters/{}/Actions.json"_format(FighterEnum(i))))
            info.actions.emplace_back(entry.get_ref<const String&>());

        for (const auto& entry : sq::parse_json_from_file("assets/fighters/{}/States.json"_format(FighterEnum(i))))
            info.states.emplace_back(entry.get_ref<const String&>());
    }
}

EditorScene::~EditorScene()
{
    const auto& ctx = sq::VulkanContext::get();
    ctx.device.waitIdle();

    ctx.device.destroy(mImageProcessSetLayout);
    ctx.device.destroy(mImageProcessPipelineLayout);
}

//============================================================================//

void EditorScene::handle_event(sq::Event event)
{
    if (event.type == sq::Event::Type::Keyboard_Press || event.type == sq::Event::Type::Keyboard_Repeat)
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

    if (event.type == sq::Event::Type::Keyboard_Press)
    {
        if (event.data.keyboard.key == sq::Keyboard_Key::Escape)
        {
            const auto predicate = [](const auto& item) { return item.second.modified; };
            mConfirmQuitNumUnsaved = uint(algo::count_if(mActionContexts, predicate) + algo::count_if(mHurtblobsContexts, predicate));
            if (mConfirmQuitNumUnsaved == 0u)
                mSmashApp.return_to_main_menu();
        }

        if (event.data.keyboard.ctrl && event.data.keyboard.key == sq::Keyboard_Key::S)
        {
            if (mActiveActionContext && mActiveActionContext->modified)
                save_changes(*mActiveActionContext);

            if (mActiveHurtblobsContext && mActiveHurtblobsContext->modified)
                save_changes(*mActiveHurtblobsContext);
        }
    }

    if (event.type == sq::Event::Type::Mouse_Scroll && event.data.scroll.delta.y != 0.f)
    {
        if (mActiveContext != nullptr)
        {
            auto& camera = static_cast<EditorCamera&>(mActiveContext->renderer->get_camera());
            camera.update_from_scroll(event.data.scroll.delta.y);
        }
    }
}

//============================================================================//

void EditorScene::refresh_options_destroy()
{
    if (mActiveContext != nullptr)
        mActiveContext->renderer->refresh_options_destroy();
}

void EditorScene::refresh_options_create()
{
    if (mActiveContext != nullptr)
        mActiveContext->renderer->refresh_options_create();
}

//============================================================================//

void EditorScene::update()
{
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

    if (mActiveHurtblobsContext != nullptr)
    {
        apply_working_changes(*mActiveHurtblobsContext);
    }
}

//============================================================================//

void EditorScene::integrate(double /*elapsed*/, float blend)
{
    if (mActiveContext == nullptr)
        return;

    if (mPreviewMode == PreviewMode::Pause)
        blend = mBlendValue;

    BaseContext& ctx = *mActiveContext;

    //--------------------------------------------------------//

    if (ImGui::GetIO().WantCaptureMouse == false)
    {
        const Vec2I position = mSmashApp.get_input_devices().get_cursor_location(true);
        const Vec2I windowSize = Vec2I(mSmashApp.get_window().get_size());

        if (position.x >= 0 && position.y >= 0 && position.x < windowSize.x && position.y < windowSize.y)
        {
            const bool leftPressed = mSmashApp.get_input_devices().is_pressed(sq::Mouse_Button::Left);
            const bool rightPressed = mSmashApp.get_input_devices().is_pressed(sq::Mouse_Button::Right);

            auto& camera = static_cast<EditorCamera&>(ctx.renderer->get_camera());

            camera.update_from_mouse(leftPressed, rightPressed, Vec2F(position));
        }
    }

    //--------------------------------------------------------//

    ctx.renderer->integrate_camera(blend);
    ctx.world->integrate(blend);

    ctx.renderer->integrate_particles(blend, ctx.world->get_particle_system());
    ctx.renderer->integrate_debug(blend, *ctx.world);

    //if (mPreviewMode != PreviewMode::Pause)
    //    mController->integrate();
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
        mDoResetDockEffects = true;
        mDoResetDockEmitters = true;
        mDoResetDockSounds = true;
        mDoResetDockScript = true;
        mDoResetDockTimeline = true;
        mDoResetDockHurtblobs = true;
        mDoResetDockStage = true;
        mDoResetDockCubemaps = true;
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
            {
                fighter.initialise_animations();
                mActiveActionContext->timelineLength = get_default_timeline_length(*mActiveActionContext);
                scrub_to_frame_current(*mActiveActionContext);
            }
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
//                        mPreviewAnimation.anim.frameCount += 48u;
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
        ImPlus::DragValue("##blend", mBlendValue, 0.01f, 0.f, 1.f, "%.2f");
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
            sq::VulkanContext::get().device.waitIdle();
            mSmashApp.get_window().set_title("SuperTuxSmash - Editor");
            mActiveContext = mActiveActionContext = nullptr;
        }
        mActionContexts.erase(ctx.key);
    };

    const auto do_close_hurtblobs = [this](HurtblobsContext& ctx)
    {
        if (mActiveContext == &ctx)
        {
            sq::VulkanContext::get().device.waitIdle();
            mSmashApp.get_window().set_title("SuperTuxSmash - Editor");
            mActiveContext = mActiveHurtblobsContext = nullptr;
        }
        mHurtblobsContexts.erase(ctx.key);
    };

    const auto do_close_stage = [this](StageContext& ctx)
    {
        if (mActiveContext == &ctx)
        {
            sq::VulkanContext::get().device.waitIdle();
            mSmashApp.get_window().set_title("SuperTuxSmash - Editor");
            mActiveContext = mActiveStageContext = nullptr;
        }
        mStageContexts.erase(ctx.key);
    };

    //--------------------------------------------------------//

    const auto action_list_entry = [&](StringView fighterName, ActionKey key)
    {
        const auto iter = mActionContexts.find(key);

        const bool loaded = iter != mActionContexts.end();
        const bool modified = loaded && iter->second.modified;
        const bool active = loaded && &iter->second == mActiveContext;

        const String label = sq::build_string(fighterName, '/', key.name, modified ? " *" : "");
        const bool highlight = ImPlus::IsPopupOpen(label);

        if (loaded) ImPlus::PushFont(ImPlus::FONT_BOLD);
        if (ImPlus::Selectable(label, active || highlight) && !active)
        {
            activate_action_context(key);
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
    };

    //--------------------------------------------------------//

    ImPlus::if_TabBar("tabbar", ImGuiTabBarFlags_FittingPolicyScroll, [&]()
    {
        ImPlus::if_TabItemChild("Actions", 0, [&]()
        {
            for (int8_t i = 0; i < sq::enum_count_v<FighterEnum>; ++i)
            {
                const FighterInfo& info = mFighterInfos[i];
                const StringView fighterName = sq::enum_to_string(FighterEnum(i));

                const size_t numLoaded = algo::count_if (
                    mActionContexts, [i](const auto& item) { return item.first.fighter == FighterEnum(i); }
                );
                const size_t numTotal = mFighterInfoCommon.actions.size() + info.actions.size();

                if (ImPlus::CollapsingHeader("{} ({}/{})###{}"_format(fighterName, numLoaded, numTotal, fighterName)))
                {
                    const ImPlus::ScopeFont font = ImPlus::FONT_MONO;

                    for (const auto& actionName : mFighterInfoCommon.actions)
                        action_list_entry(fighterName, {FighterEnum(i), actionName});

                    ImGui::Separator();

                    for (const auto& actionName : info.actions)
                        action_list_entry(fighterName, {FighterEnum(i), actionName});
                }
            }
        });

        ImPlus::if_TabItemChild("HurtBlobs", 0, [&]()
        {
            for (int8_t i = 0; i < sq::enum_count_v<FighterEnum>; ++i)
            {
                const FighterEnum key = FighterEnum(i);

                const auto iter = mHurtblobsContexts.find(key);

                const bool loaded = iter != mHurtblobsContexts.end();
                const bool modified = loaded && iter->second.modified;
                const bool active = loaded && &iter->second == mActiveContext;

                const String label = sq::build_string(sq::enum_to_string(key), modified ? " *" : "");
                const bool highlight = ImPlus::IsPopupOpen(label);

                if (loaded) ImPlus::PushFont(ImPlus::FONT_BOLD);
                if (ImPlus::Selectable(label, active || highlight) && !active)
                {
                    mSmashApp.get_window().set_title("SuperTuxSmash - Editor - {} (HurtBlobs)"_format(key));

                    mActiveActionContext = nullptr;
                    mActiveStageContext = nullptr;
                    mActiveContext = mActiveHurtblobsContext = &get_hurtblobs_context(key);
                    mActiveContext->renderer->refresh_options_destroy();
                    mActiveContext->renderer->refresh_options_create();
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

        ImPlus::if_TabItemChild("Stages", 0, [&]()
        {
            for (int8_t i = 0; i < sq::enum_count_v<StageEnum>; ++i)
            {
                const StageEnum key = StageEnum(i);

                const auto iter = mStageContexts.find(key);

                const bool loaded = iter != mStageContexts.end();
                const bool modified = loaded && iter->second.modified;
                const bool active = loaded && &iter->second == mActiveContext;

                const String label = sq::build_string(sq::enum_to_string(key), modified ? " *" : "");
                const bool highlight = ImPlus::IsPopupOpen(label);

                if (loaded) ImPlus::PushFont(ImPlus::FONT_BOLD);
                if (ImPlus::Selectable(label, active || highlight) && !active)
                {
                    mSmashApp.get_window().set_title("SuperTuxSmash - Editor - {} (Stage)"_format(key));

                    mActiveActionContext = nullptr;
                    mActiveHurtblobsContext = nullptr;
                    mActiveContext = mActiveStageContext = &get_stage_context(key);
                    mActiveContext->renderer->refresh_options_destroy();
                    mActiveContext->renderer->refresh_options_create();
                }
                if (loaded) ImGui::PopFont();

                ImPlus::if_PopupContextItem(nullptr, ImPlus::MOUSE_RIGHT, [&]()
                {
                    if (ImGui::MenuItem("Close", nullptr, false, loaded))
                    {
                        if (modified) mConfirmCloseStageCtx = &iter->second;
                        else do_close_stage(iter->second);
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
        if (result == ImPlus::DialogResult::Cancel) mConfirmCloseActionCtx = nullptr;
    }

    if (mConfirmCloseHurtblobsCtx != nullptr)
    {
        const auto result = ImPlus::DialogConfirmation("Discard Changes", "HurtBlobs have been modified, really discard changes?");
        if (result == ImPlus::DialogResult::Confirm) do_close_hurtblobs(*mConfirmCloseHurtblobsCtx);
        if (result == ImPlus::DialogResult::Cancel) mConfirmCloseHurtblobsCtx = nullptr;
    }

    if (mConfirmCloseStageCtx != nullptr)
    {
        const auto result = ImPlus::DialogConfirmation("Discard Changes", "Stage has been modified, really discard changes?");
        if (result == ImPlus::DialogResult::Confirm) do_close_stage(*mConfirmCloseStageCtx);
        if (result == ImPlus::DialogResult::Cancel) mConfirmCloseStageCtx = nullptr;
    }

    //--------------------------------------------------------//

    if (mConfirmQuitNumUnsaved != 0u)
    {
        const auto result = ImPlus::DialogConfirmation("Confirm Quit", mConfirmQuitNumUnsaved > 1u ?
                                                       "{} objects have not been saved, really quit?"_format(mConfirmQuitNumUnsaved) :
                                                       "1 object has not been saved, really quit?");
        if (result == ImPlus::DialogResult::Confirm)
        {
            mActionContexts.clear();
            mHurtblobsContexts.clear();
            mStageContexts.clear();
            mSmashApp.return_to_main_menu();
        }
        if (result == ImPlus::DialogResult::Cancel) mConfirmQuitNumUnsaved = 0u;
    }
}

//============================================================================//

void EditorScene::impl_show_widget_fighter()
{
    // todo: make proper widget for editing and saving attributes, maybe merge with HurtBlobs editor?

    Fighter* fighter = nullptr;
    if (mActiveActionContext != nullptr) fighter = mActiveActionContext->fighter;
    if (mActiveHurtblobsContext != nullptr) fighter = mActiveHurtblobsContext->fighter;
    if (fighter == nullptr) return;

    if (mDoResetDockFighter) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockFighter = false;

    const ImPlus::ScopeWindow window = { "Fighter", 0 };
    if (window.show == false) return;

    DebugGui::show_widget_stage(mActiveActionContext->world->get_stage());
    DebugGui::show_widget_fighter(*fighter);
}

//============================================================================//

void EditorScene::show_imgui_widgets()
{
    impl_show_widget_toolbar();
    impl_show_widget_navigator();

    if (mActiveActionContext != nullptr)
    {
        impl_show_widget_hitblobs();
        impl_show_widget_emitters();
        impl_show_widget_sounds();
        impl_show_widget_effects();
        impl_show_widget_script();
        impl_show_widget_timeline();
        impl_show_widget_fighter();
    }
    else if (mActiveHurtblobsContext != nullptr)
    {
        impl_show_widget_hurtblobs();
        impl_show_widget_fighter();
    }
    else if (mActiveStageContext != nullptr)
    {
        impl_show_widget_stage();
        impl_show_widget_cubemaps();
    }
}

//============================================================================//

uint EditorScene::get_default_timeline_length(const ActionContext& ctx)
{
    // todo: jsonify all of this so it can properly support extra actions

    const auto anim_length = [&ctx](const SmallString& name)
    {
        const auto iter = ctx.fighter->mAnimations.find(name);
        if (iter == ctx.fighter->mAnimations.end())
        {
            sq::log_warning("could not find animation '{}'", name);
            return 80u; // too short for some actions, but annoyingly long for others
        }
        return iter->second.anim.frameCount;
    };

    uint result = 1u; // extra time before looping

    if      (ctx.key.name == "HopBack")    result += anim_length("JumpBack");
    else if (ctx.key.name == "HopForward") result += anim_length("JumpForward");
    else                                   result += anim_length(ctx.key.name);

    return result;
}

//============================================================================//

void EditorScene::initialise_base_context(BaseContext& ctx)
{
    sq::Window& window = mSmashApp.get_window();
    sq::AudioContext& audioContext = mSmashApp.get_audio_context();

    Options& options = mSmashApp.get_options();
    ResourceCaches& resourceCaches = mSmashApp.get_resource_caches();

    ctx.renderer = std::make_unique<Renderer>(window, options, resourceCaches);
    ctx.world = std::make_unique<FightWorld>(options, audioContext, resourceCaches, *ctx.renderer);

    ctx.renderer->set_camera(std::make_unique<EditorCamera>(*ctx.renderer));
}

//----------------------------------------------------------------------------//

void EditorScene::activate_action_context(ActionKey key)
{
    mSmashApp.get_window().set_title("SuperTuxSmash - Editor - {}/{}"_format(key.fighter, key.name));

    mActiveHurtblobsContext = nullptr;
    mActiveStageContext = nullptr;

    if (auto iter = mActionContexts.find(key); iter != mActionContexts.end())
    {
        mActiveContext = mActiveActionContext = &iter->second;
        mActiveContext->renderer->refresh_options_destroy();
        mActiveContext->renderer->refresh_options_create();
        return; // already loaded
    }

    ActionContext& ctx = mActionContexts[key];
    initialise_base_context(ctx);

    ctx.world->set_stage(std::make_unique<Stage>(*ctx.world, StageEnum::TestZone));
    ctx.world->add_fighter(std::make_unique<Fighter>(*ctx.world, key.fighter, 0u));
    ctx.world->get_fighter(0u).controller = mController.get();

    ctx.key = key;
    ctx.fighter = &ctx.world->get_fighter(0u);
    ctx.action = &ctx.fighter->mActions.at(ctx.key.name);

    scrub_to_frame_current(ctx);

    ctx.savedData = ctx.action->clone();
    ctx.undoStack.push_back(ctx.action->clone());

    ctx.timelineLength = get_default_timeline_length(ctx);

    mActiveContext = mActiveActionContext = &ctx;
}

//----------------------------------------------------------------------------//

EditorScene::HurtblobsContext& EditorScene::get_hurtblobs_context(FighterEnum key)
{
    if (auto iter = mHurtblobsContexts.find(key); iter != mHurtblobsContexts.end())
        return iter->second;

    HurtblobsContext& ctx = mHurtblobsContexts[key];
    initialise_base_context(ctx);

    ctx.world->set_stage(std::make_unique<Stage>(*ctx.world, StageEnum::TestZone));
    ctx.world->add_fighter(std::make_unique<Fighter>(*ctx.world, key, 0u));

    ctx.key = key;
    ctx.fighter = &ctx.world->get_fighter(0u);

    //ctx.fighter->change_state(FighterState::EditorPreview, true);
    sq::log_error("todo: fix hurtblob editor");
    ctx.world->tick();

    ctx.savedData = std::make_unique<decltype(Fighter::mHurtBlobs)>(ctx.fighter->mHurtBlobs);
    ctx.undoStack.push_back(std::make_unique<decltype(Fighter::mHurtBlobs)>(ctx.fighter->mHurtBlobs));

    return ctx;
}

//----------------------------------------------------------------------------//

EditorScene::StageContext& EditorScene::get_stage_context(StageEnum key)
{
    if (auto iter = mStageContexts.find(key); iter != mStageContexts.end())
        return iter->second;

    StageContext& ctx = mStageContexts[key];
    initialise_base_context(ctx);

    ctx.world->set_stage(std::make_unique<Stage>(*ctx.world, key));

    ctx.key = key;
    ctx.stage = &ctx.world->get_stage();

    ctx.skybox.initialise(*this, 0u, ctx.renderer->cubemaps.skybox.get_image(), ctx.renderer->samplers.linearClamp);
    ctx.irradiance.initialise(*this, 0u, ctx.renderer->cubemaps.irradiance.get_image(), ctx.renderer->samplers.linearClamp);

    for (uint level = 0u; level < ctx.radiance.size(); ++level)
        ctx.radiance[level].initialise(*this, level, ctx.renderer->cubemaps.radiance.get_image(), ctx.renderer->samplers.linearClamp);

    //ctx.savedData = std::make_unique<decltype(Fighter::mHurtBlobs)>(ctx.fighter->mHurtBlobs);
    //ctx.undoStack.push_back(std::make_unique<decltype(Fighter::mHurtBlobs)>(ctx.fighter->mHurtBlobs));

    return ctx;
}

//============================================================================//

// TODO: should merge similar edits so that dragging a slider doesn't generate 50+ undo entries
// probably need to make some "can_merge_edits" methods for editables and to store the time for each record

void EditorScene::apply_working_changes(ActionContext& ctx)
{
    FighterAction& action = *ctx.action;

    if (action.has_changes(*ctx.undoStack[ctx.undoIndex]) == true)
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

void EditorScene::apply_working_changes(StageContext& ctx)
{
    //if (ctx.fighter->mHurtBlobs != *ctx.undoStack[ctx.undoIndex])
    {
        ctx.world->tick();

        //ctx.undoStack.erase(ctx.undoStack.begin() + ++ctx.undoIndex, ctx.undoStack.end());
        //ctx.undoStack.push_back(std::make_unique<decltype(Fighter::mHurtBlobs)>(ctx.fighter->mHurtBlobs));

        ctx.modified = true;//ctx.fighter->mHurtBlobs != *ctx.savedData;
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

        FighterAction& action = *ctx.action;
        action.apply_changes(*ctx.undoStack[ctx.undoIndex]);
        action.load_wren_from_string();

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
    const Fighter& fighter = *ctx.fighter;
    const FighterAction& action = *ctx.action;

    JsonValue json;

    auto& blobs = json["blobs"] = JsonValue::object();
    auto& effects = json["effects"] = JsonValue::object();
    auto& emitters = json["emitters"] = JsonValue::object();
    auto& sounds = json["sounds"] = JsonValue::object();

    for (const auto& [key, blob] : action.mBlobs)
        blob.to_json(blobs[key.c_str()]);

    for (const auto& [key, effect] : action.mEffects)
        effect.to_json(effects[key.c_str()]);

    for (const auto& [key, emitter] : action.mEmitters)
        emitter.to_json(emitters[key.c_str()]);

    for (const auto& [key, sound] : action.mSounds)
        sound.to_json(sounds[key.c_str()]);

    sq::write_text_to_file("assets/fighters/{}/actions/{}.json"_format(fighter.name, action.name), json.dump(2));
    sq::write_text_to_file("assets/fighters/{}/actions/{}.wren"_format(fighter.name, action.name), action.mWrenSource);

    ctx.savedData = action.clone();
    ctx.modified = false;
}

void EditorScene::save_changes(HurtblobsContext& ctx)
{
    const Fighter& fighter = *ctx.fighter;

    JsonValue json;

    for (const auto& [key, blob] : fighter.mHurtBlobs)
        blob.to_json(json[key.c_str()]);

    sq::write_text_to_file("assets/fighters/{}/HurtBlobs.json"_format(fighter.name), json.dump(2));

    *ctx.savedData = fighter.mHurtBlobs;
    ctx.modified = false;
}

void EditorScene::save_changes(StageContext& ctx)
{
    ctx.modified = false;
}

//============================================================================//

void EditorScene::scrub_to_frame(ActionContext& ctx, int frame)
{
    // wait for all in progress rendering to finish
    sq::VulkanContext::get().device.waitIdle();

    // now we can reset everything

    //mController->wren_clear_history();

    ctx.world->get_particle_system().reset_random_seed(mRandomSeed);
    ctx.world->get_particle_system().clear();

    ctx.world->get_effect_system().clear();

    Fighter& fighter = *ctx.fighter;

    fighter.reset_everything();

    auto& attrs = fighter.attributes;
    auto& vars = fighter.variables;

    //--------------------------------------------------------//

    // some actions have different starting state
    // todo: this should be moved into wren so you can add new action types
    // todo: manipulate input to have the action start naturally

    if (ctx.key.name == "Brake" || ctx.key.name == "BrakeTurn")
    {
        //fighter.change_state(fighter.mStates.at("Dash"));
        fighter.change_state(fighter.mStates.at("Neutral"));
        fighter.play_animation(fighter.mAnimations.at("DashLoop"), 0u, true);
        vars.velocity.x = attrs.dashSpeed + attrs.traction;
    }

    else if (ctx.key.name == "HopBack" || ctx.key.name == "HopForward")
    {
        fighter.change_state(fighter.mStates.at("JumpSquat"));
        fighter.play_animation(fighter.mAnimations.at("JumpSquat"), 0u, true);
        vars.velocity.y = std::sqrt(2.f * attrs.hopHeight * attrs.gravity) + attrs.gravity * 0.5f;
    }

    else if (ctx.key.name == "JumpBack" || ctx.key.name == "JumpForward")
    {
        fighter.change_state(fighter.mStates.at("JumpSquat"));
        fighter.play_animation(fighter.mAnimations.at("JumpSquat"), 0u, true);
        vars.velocity.y = std::sqrt(2.f * attrs.jumpHeight * attrs.gravity) + attrs.gravity * 0.5f;
    }

    else if (ctx.key.name.starts_with("Air") || ctx.key.name.starts_with("SpecialAir"))
    {
        fighter.change_state(fighter.mStates.at("Fall"));
        fighter.play_animation(fighter.mAnimations.at("FallLoop"), 0u, true);
        attrs.gravity = 0.f;
        vars.position = Vec2F(0.f, 1.f);
    }

    else
    {
        fighter.change_state(fighter.mStates.at("Neutral"));
        fighter.play_animation(fighter.mAnimations.at("NeutralLoop"), 0u, true);
    }

    //--------------------------------------------------------//

    // tick once to apply neutral/falling animation
    ctx.world->tick();
    ctx.fighter->previous = ctx.fighter->current;
    ctx.fighter->debugPreviousPoseInfo = ctx.fighter->debugCurrentPoseInfo;

    // will activate the action when ctx.currentFrame >= 0
    fighter.editorStartAction = ctx.action;

    // finally, scrub to the desired frame
    mSmashApp.get_audio_context().set_groups_ignored(sq::SoundGroup::Sfx, true);
    for (ctx.currentFrame = -1; ctx.currentFrame < frame;)
        tick_action_context(ctx);
    mSmashApp.get_audio_context().set_groups_ignored(sq::SoundGroup::Sfx, false);

    // this would be an assertion, but we want to avoid crashing the editor
    #ifdef SQEE_DEBUG
    if (fighter.activeAction != nullptr && ctx.currentFrame >= 0 && fighter.editorErrorCause == nullptr)
    {
        const int actionFrame = int(fighter.activeAction->mCurrentFrame) - 1;
        if (ctx.currentFrame != actionFrame)
            sq::log_warning("out of sync: timeline = {}, action = {}", ctx.currentFrame, actionFrame);
    }
    #endif
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
    else
    {
        //mController->tick();
        ctx.world->tick();
    }
}

void EditorScene::scrub_to_frame_previous(ActionContext& ctx)
{
    if (ctx.currentFrame == -1) scrub_to_frame(ctx, ctx.timelineLength - 1);
    else scrub_to_frame(ctx, ctx.currentFrame - 1);
}

//============================================================================//

void EditorScene::populate_command_buffer(vk::CommandBuffer cmdbuf, vk::Framebuffer framebuf)
{
    const Vec2U windowSize = mSmashApp.get_window().get_size();

    if (mActiveContext != nullptr)
        mActiveContext->renderer->populate_command_buffer(cmdbuf);

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            mSmashApp.get_window().get_render_pass(), framebuf, vk::Rect2D({0, 0}, {windowSize.x, windowSize.y})
        }, vk::SubpassContents::eInline
    );

    if (mActiveContext != nullptr)
        mActiveContext->renderer->populate_final_pass(cmdbuf);
    else
        cmdbuf.clearAttachments (
            vk::ClearAttachment(vk::ImageAspectFlagBits::eColor, 0u, vk::ClearValue(vk::ClearColorValue().setFloat32({}))),
            vk::ClearRect(vk::Rect2D({0, 0}, {windowSize.x, windowSize.y}), 0u, 1u)
        );

    mSmashApp.get_gui_system().render_gui(cmdbuf);

    cmdbuf.endRenderPass();
}

//============================================================================//

const void* EditorScene::ActionKey::hash() const
{
    // terrible hash combine, but ok for this
    const size_t nameHash = std::hash<SmallString>()(name);
    const size_t comboHash = nameHash + size_t(fighter);

    // cast to a type that imgui can hash
    return reinterpret_cast<const void*>(comboHash);
}

//============================================================================//

void EditorScene::CubeMapView::initialise(EditorScene& editor, uint level, vk::Image image, vk::Sampler sampler)
{
    const auto& ctx = sq::VulkanContext::get();

    for (uint face = 0u; face < 6u; ++face)
    {
        if (imageViews[face]) ctx.device.destroy(imageViews[face]);

        imageViews[face] = ctx.create_image_view (
            image, vk::ImageViewType::e2D, vk::Format::eE5B9G9R9UfloatPack32,
            vk::ComponentMapping(), vk::ImageAspectFlagBits::eColor, level, 1u, face, 1u
        );

        if (!descriptorSets[face])
            descriptorSets[face] = ctx.allocate_descriptor_set (
                ctx.descriptorPool, editor.mSmashApp.get_gui_system().get_descriptor_set_layout()
            );

        sq::vk_update_descriptor_set (
            ctx, descriptorSets[face],
            sq::DescriptorImageSampler(0u, 0u, sampler, imageViews[face], vk::ImageLayout::eShaderReadOnlyOptimal)
        );
    }
}

EditorScene::StageContext::~StageContext()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.free(ctx.descriptorPool, skybox.descriptorSets);
    for (auto& imageView : skybox.imageViews)
        ctx.device.destroy(imageView);

    ctx.device.free(ctx.descriptorPool, irradiance.descriptorSets);
    for (auto& imageView : irradiance.imageViews)
        ctx.device.destroy(imageView);

    for (auto& radianceLevel : radiance)
    {
        ctx.device.free(ctx.descriptorPool, radianceLevel.descriptorSets);
        for (auto& imageView : radianceLevel.imageViews)
            ctx.device.destroy(imageView);
    }
}
