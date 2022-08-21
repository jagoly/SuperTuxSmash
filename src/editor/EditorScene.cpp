#include "editor/EditorScene.hpp"

#include "main/Options.hpp"
#include "main/SmashApp.hpp"

#include "game/Controller.hpp"
#include "game/Stage.hpp"
#include "game/World.hpp"

#include "render/Renderer.hpp"

#include "editor/EditorCamera.hpp"
#include "editor/Editor_Action.hpp"
#include "editor/Editor_Fighter.hpp"
#include "editor/Editor_Stage.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiWidgets.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/vk/Helpers.hpp>
#include <sqee/vk/VulkanContext.hpp>

using namespace sts;

//============================================================================//

constexpr const float HEIGHT_TIMELINE = 80.f;
constexpr const float WIDTH_NAVIGATOR = 240.f;
constexpr const float WIDTH_EDITORS = 480.f;

//============================================================================//

EditorScene::EditorScene(SmashApp& smashApp)
    : Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    auto& options = mSmashApp.get_options();

    options.render_hit_blobs = true;
    options.render_hurt_blobs = true;
    options.render_diamonds = true;
    options.render_skeletons = true;

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

    const auto load_fighter_info = [](const String& name)
    {
        FighterInfo result;

        const String path = name.empty() ? "assets/fighters" : ("assets/fighters/" + name);

        result.name = TinyString(name);
        {
            const auto json = sq::parse_json_from_file(path + "/Animations.json");
            result.animations.reserve(json.size());
            for (const auto& entry : json)
                result.animations.emplace_back(entry[0].get_ref<const String&>());
        }
        {
            const auto json = sq::parse_json_from_file(path + "/Actions.json");
            result.actions.reserve(json.size());
            for (const auto& entry : json)
                result.actions.emplace_back(entry.get_ref<const String&>());
        }
        {
            const auto json = sq::parse_json_from_file(path + "/States.json");
            result.states.reserve(json.size());
            for (const auto& entry : json)
                result.states.emplace_back(entry.get_ref<const String&>());
        }

        return result;
    };

    mFighterInfoCommon = load_fighter_info(String());
    {
        const auto json = sq::parse_json_from_file("assets/fighters/Fighters.json");
        mFighterInfos.reserve(json.size());
        for (const auto& entry : json)
            mFighterInfos.emplace_back(load_fighter_info(entry.get_ref<const String&>()));
    }

    {
        const auto json = sq::parse_json_from_file("assets/stages/Stages.json");
        mStageNames.reserve(json.size());
        for (const auto& entry : json)
            mStageNames.emplace_back(entry.get_ref<const String&>());
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
    const bool press = event.type == sq::Event::Type::Keyboard_Press;
    const bool repeat = event.type == sq::Event::Type::Keyboard_Repeat;
    const auto& kb = event.data.keyboard;

    if (event.type == sq::Event::Type::Window_Close)
    {
        impl_confirm_quit_unsaved(false);
    }

    else if ((press || repeat) && kb.key == sq::Keyboard_Key::Space)
    {
        if (mPreviewMode == PreviewMode::Pause)
        {
            if (auto ctx = dynamic_cast<ActionContext*>(mActiveContext))
                ctx->advance_frame(kb.shift);
        }
        else mPreviewMode = PreviewMode::Pause;
    }

    else if ((press || repeat) && kb.ctrl && kb.key == sq::Keyboard_Key::Z)
    {
        if (mActiveContext != nullptr)
            mActiveContext->do_undo_redo(kb.shift);
    }

    else if (press && kb.key == sq::Keyboard_Key::Escape)
    {
        impl_confirm_quit_unsaved(true);
    }

    else if (press && kb.ctrl && kb.key == sq::Keyboard_Key::S)
    {
        if (mActiveContext != nullptr && mActiveContext->modified == true)
            mActiveContext->save_changes();
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
    if (mActiveContext != nullptr)
    {
        mActiveContext->apply_working_changes();

        if (mPreviewMode != PreviewMode::Pause)
            if (auto ctx = dynamic_cast<ActionContext*>(mActiveContext))
                ctx->advance_frame(false);
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

    ctx.renderer->swap_objects_buffers();

    ctx.renderer->get_camera().update_from_controller(*mController);

    ctx.renderer->integrate_camera(blend);
    ctx.world->integrate(blend);

    ctx.renderer->integrate_particles(blend, ctx.world->get_particle_system());
    ctx.renderer->integrate_debug(blend, *ctx.world);

    //if (mPreviewMode != PreviewMode::Pause)
    //    mController->integrate();
}

//============================================================================//

void EditorScene::impl_confirm_quit_unsaved(bool returnToMenu)
{
    const auto append_unsaved_items = [this](const auto& ctxMap)
    {
        for (const auto& [key, ctx] : ctxMap)
            if (ctx.modified == true)
                sq::format_append(mConfirmQuitUnsaved, "  - {} ({})\n", ctx.ctxKeyString, ctx.ctxTypeString);
    };

    append_unsaved_items(mActionContexts);
    append_unsaved_items(mFighterContexts);
    append_unsaved_items(mStageContexts);

    if (mConfirmQuitUnsaved.empty())
    {
        if (returnToMenu) mSmashApp.return_to_main_menu();
        else mSmashApp.quit();
    }
    else mConfirmQuitReturnToMenu = returnToMenu;
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
        mDoResetDockScript = true;
        mDoResetDockTimeline = true;
        mDoResetDockHurtblobs = true;
        mDoResetDockSounds = true;
        mDoResetDockStage = true;
        mDoResetDockCubemaps = true;
        mDoResetDockDebug = true;

        const auto viewSize = ImGui::GetWindowViewport()->Size;

        ImGui::DockBuilderRemoveNode(mDockMainId);
        ImGui::DockBuilderAddNode(mDockMainId, 1 << 10);
        ImGui::DockBuilderSetNodeSize(mDockMainId, viewSize);

        ImGui::DockBuilderSplitNode(mDockMainId, ImGuiDir_Down, 0.2f, &mDockDownId, &mDockNotDownId);
        ImGui::DockBuilderSplitNode(mDockNotDownId, ImGuiDir_Left, 0.2f, &mDockLeftId, &mDockNotLeftId);
        ImGui::DockBuilderSplitNode(mDockNotLeftId, ImGuiDir_Right, 0.2f, &mDockRightId, &mDockNotRightId);

        ImGui::DockBuilderSetNodeSize(mDockDownId, {viewSize.x, HEIGHT_TIMELINE});
        ImGui::DockBuilderSetNodeSize(mDockLeftId, {WIDTH_NAVIGATOR, viewSize.y});
        ImGui::DockBuilderSetNodeSize(mDockRightId, {WIDTH_EDITORS, viewSize.y});

        ImGui::DockBuilderFinish(mDockMainId);
    }
    mWantResetDocks = false;

    ImGui::DockSpace (
        mDockMainId, ImVec2(0.0f, 0.0f),
        ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode
    );

    if (window.show == false) return;

    //--------------------------------------------------------//

    ImPlus::if_MenuBar([&]()
    {
        ImPlus::if_Menu("Menu", true, [&]()
        {
            if (ImPlus::MenuItem("Reset Layout"))
                mWantResetDocks = true;

            if (ImPlus::MenuItem("Save", "Ctrl+S", false, mActiveContext && mActiveContext->modified))
                mActiveContext->save_changes();

            const size_t numModified =
                ranges::count_if(mActionContexts, [](const auto& item) { return item.second.modified; }) +
                ranges::count_if(mFighterContexts, [](const auto& item) { return item.second.modified; }) +
                ranges::count_if(mStageContexts, [](const auto& item) { return item.second.modified; });

            if (ImPlus::MenuItem(fmt::format("Save All ({})", numModified), "Ctrl+Shift+S", false, numModified != 0u))
            {
                // todo: show a popup listing everything that has changed
                for (auto& [key, ctx] : mActionContexts) if (ctx.modified) ctx.save_changes();
                for (auto& [key, ctx] : mFighterContexts) if (ctx.modified) ctx.save_changes();
                for (auto& [key, ctx] : mStageContexts) if (ctx.modified) ctx.save_changes();
            }

            if (mActiveContext != nullptr)
            {
                ImGui::Separator();
                mActiveContext->show_menu_items();
            }
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

        ImGui::Separator();

        // todo: move this stuff into ActionContext

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
            if (auto ctx = dynamic_cast<ActionContext*>(mActiveContext))
                ctx->scrub_to_frame(ctx->currentFrame);
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

    const auto activate_context = [this](const auto& ctxKey, auto& ctxMap)
    {
        auto [iter, created] = ctxMap.try_emplace(ctxKey, *this, ctxKey);
        mActiveContext = &iter->second;

        if (created == false)
        {
            mActiveContext->renderer->refresh_options_destroy();
            mActiveContext->renderer->refresh_options_create();
        }

        mSmashApp.get_window().set_title (
            fmt::format("SuperTuxSmash - Editor - {} ({})", mActiveContext->ctxKeyString, mActiveContext->ctxTypeString)
        );
    };

    const auto close_context = [this](BaseContext* baseCtx)
    {
        if (mActiveContext == baseCtx)
        {
            sq::VulkanContext::get().device.waitIdle();
            mSmashApp.get_window().set_title("SuperTuxSmash - Editor");
            mActiveContext = nullptr;
        }

        if (auto ctx = dynamic_cast<ActionContext*>(baseCtx))
            mActionContexts.erase(ctx->ctxKey);
        else if (auto ctx = dynamic_cast<FighterContext*>(baseCtx))
            mFighterContexts.erase(ctx->ctxKey);
        else if (auto ctx = dynamic_cast<StageContext*>(baseCtx))
            mStageContexts.erase(ctx->ctxKey);
        else SQEE_UNREACHABLE();

        mConfirmCloseContext = nullptr;
    };

    const auto context_list_entry = [&](const auto& ctxKey, auto& ctxMap, String label)
    {
        const auto iter = ctxMap.find(ctxKey);

        const bool loaded = iter != ctxMap.end();
        const bool modified = loaded && iter->second.modified;
        const bool active = loaded && &iter->second == mActiveContext;

        if (modified) label.push_back('*');

        const bool highlight = active || ImPlus::IsPopupOpen(label);

        if (loaded) ImPlus::PushFont(ImPlus::FONT_BOLD);
        if (ImPlus::Selectable(label, highlight) && !active) activate_context(ctxKey, ctxMap);
        if (loaded) ImGui::PopFont();

        ImPlus::if_PopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight, [&]()
        {
            if (ImGui::MenuItem("Close", nullptr, false, loaded))
            {
                if (modified) mConfirmCloseContext = &iter->second;
                else close_context(&iter->second);
            }
            if (ImGui::MenuItem("Save", nullptr, false, modified))
                iter->second.save_changes();
        });
    };

    //--------------------------------------------------------//

    ImPlus::if_TabBar("tabbar", ImGuiTabBarFlags_FittingPolicyScroll, [&]()
    {
        ImPlus::if_TabItemChild("Actions", 0, [&]()
        {
            for (const FighterInfo& info : mFighterInfos)
            {
                const size_t numLoaded = ranges::count_if (
                    mActionContexts, [&info](const auto& item) { return item.first.fighter == info.name; }
                );
                const size_t numTotal = mFighterInfoCommon.actions.size() + info.actions.size();

                if (ImPlus::CollapsingHeader(fmt::format("{} ({}/{})###{}", info.name, numLoaded, numTotal, info.name)))
                {
                    for (const SmallString& name : mFighterInfoCommon.actions)
                        context_list_entry(ActionKey{info.name, name}, mActionContexts, fmt::format("{}/{}", info.name, name));

                    ImGui::Separator();

                    for (const SmallString& name : info.actions)
                        context_list_entry(ActionKey{info.name, name}, mActionContexts, fmt::format("{}/{}", info.name, name));
                }
            }
        });

        ImPlus::if_TabItemChild("Fighters", 0, [&]()
        {
            for (const FighterInfo& info : mFighterInfos)
                context_list_entry(info.name, mFighterContexts, String(info.name));
        });

        ImPlus::if_TabItemChild("Stages", 0, [&]()
        {
            for (const TinyString& name : mStageNames)
                context_list_entry(name, mStageContexts, String(name));
        });
    });

    //--------------------------------------------------------//

    if (mConfirmCloseContext != nullptr)
    {
        const auto result = ImPlus::DialogConfirmation (
            "Discard Changes", fmt::format("{} modified, really discard changes?", mConfirmCloseContext->ctxTypeString)
        );

        if (result == ImPlus::DialogResult::Confirm)
            close_context(mConfirmCloseContext);

        if (result == ImPlus::DialogResult::Cancel)
            mConfirmCloseContext = nullptr;
    }

    if (mConfirmQuitUnsaved.empty() == false)
    {
        const auto result = ImPlus::DialogConfirmation (
            "Discard Changes", fmt::format("Some items have not been saved:\n{}Really quit without saving?", mConfirmQuitUnsaved)
        );

        if (result == ImPlus::DialogResult::Confirm)
        {
            if (mConfirmQuitReturnToMenu) mSmashApp.return_to_main_menu();
            else mSmashApp.quit();
        }

        if (result == ImPlus::DialogResult::Cancel)
            mConfirmQuitUnsaved.clear();
    }
}

//============================================================================//

void EditorScene::show_imgui_widgets()
{
    impl_show_widget_toolbar();
    impl_show_widget_navigator();

    if (mActiveContext != nullptr)
        mActiveContext->show_widgets();
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

//============================================================================//

EditorScene::BaseContext::BaseContext(EditorScene& editor, TinyString stage)
    : editor(editor)
{
    sq::Window& window = editor.mSmashApp.get_window();
    sq::AudioContext& audioContext = editor.mSmashApp.get_audio_context();

    Options& options = editor.mSmashApp.get_options();
    ResourceCaches& resourceCaches = editor.mSmashApp.get_resource_caches();

    renderer = std::make_unique<Renderer>(window, options, resourceCaches);
    camera = std::make_unique<EditorCamera>(*renderer);
    renderer->set_camera(*camera);

    world = std::make_unique<World>(options, audioContext, resourceCaches, *renderer);
    world->editor = std::make_unique<World::EditorData>();

    world->set_rng_seed(editor.mRandomSeed);
    world->set_stage(std::make_unique<Stage>(*world, stage));
}

EditorScene::BaseContext::~BaseContext() = default;
