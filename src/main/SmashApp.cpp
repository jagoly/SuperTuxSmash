#include "main/SmashApp.hpp"

#include "main/Options.hpp"
#include "main/Resources.hpp"

#include "editor/EditorScene.hpp"
#include "main/GameScene.hpp"
#include "main/MenuScene.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/vk/VulkanContext.hpp>

using namespace sts;

//============================================================================//

SmashApp::SmashApp() = default;

SmashApp::~SmashApp()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.waitIdle();
}

//============================================================================//

void SmashApp::initialise(std::vector<String> /*args*/)
{
    mWindow = std::make_unique<sq::Window> (
        "SuperTuxSmash - Main Menu", Vec2U(1280u, 720u),
        "SuperTuxSmash", Vec3U(0u, 0u, 1u)
    );
    mWindow->create_swapchain_and_friends();
    mWindow->set_vsync_enabled(true);
    mWindow->set_key_repeat(false);

    mInputDevices = std::make_unique<sq::InputDevices>(*mWindow);
    mDebugOverlay = std::make_unique<sq::DebugOverlay>();
    mAudioContext = std::make_unique<sq::AudioContext>();

    mGuiSystem = std::make_unique<sq::GuiSystem>(*mWindow, *mInputDevices);
    mGuiSystem->set_style_widgets_supertux();
    mGuiSystem->set_style_colours_supertux();

    mOptions = std::make_unique<Options>();
    mResourceCaches = std::make_unique<ResourceCaches>(*mAudioContext);

    return_to_main_menu();
}

//============================================================================//

void SmashApp::update(double elapsed)
{
    SQASSERT(mActiveScene != nullptr, "we have no scene!");

    //-- fetch and handle events -----------------------------//

    if (mWindow->has_focus())
    {
        for (const auto& event : mWindow->fetch_events())
            if (!mGuiSystem->handle_event(event))
                handle_event(event);
        mGuiSystem->finish_handle_events(true);
    }
    else
    {
        void(mWindow->fetch_events());
        mGuiSystem->finish_handle_events(false);
    }

    //-- update scene and imgui ------------------------------//

    mActiveScene->update_and_integrate(elapsed);
    mDebugOverlay->update_and_integrate(elapsed);

    mActiveScene->show_imgui_widgets();
    mDebugOverlay->show_imgui_widgets();

    if (mOptions->imgui_demo)
        mGuiSystem->show_imgui_demo();

    mGuiSystem->finish_scene_update(elapsed);

    //-- populate and submit command buffer ------------------//

    auto [cmdbuf, framebuf] = mWindow->begin_frame();
    if (cmdbuf)
    {
        populate_command_buffer(cmdbuf, framebuf);
        mWindow->submit_present_swap();
    }
}

//============================================================================//

void SmashApp::populate_command_buffer(vk::CommandBuffer cmdbuf, vk::Framebuffer framebuf)
{
    cmdbuf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr});
    mActiveScene->populate_command_buffer(cmdbuf, framebuf);
    cmdbuf.end();
}

//============================================================================//

void SmashApp::handle_event(sq::Event event)
{
    using Type = sq::Event::Type;
    using Key = sq::Keyboard_Key;

    const auto& [type, data] = event;

    //--------------------------------------------------------//

    if (type == Type::Window_Close)
    {
        mReturnCode = 0;
        return;
    }

    if (type == Type::Window_Resize)
    {
        refresh_options();
        return;
    }

    //--------------------------------------------------------//

    if (type == Type::Keyboard_Press)
    {
        if (data.keyboard.key == Key::Menu)
        {
            mDebugOverlay->toggle_active();
            return;
        }

        if (data.keyboard.key == Key::Escape)
        {
            return_to_main_menu();
            return;
        }
    }

    //--------------------------------------------------------//

    if (type == Type::Keyboard_Press)
    {
        if (data.keyboard.key == Key::V)
        {
            constexpr const auto STRINGS = std::array { "OFF", "ON" };
            const bool newValue = !mWindow->get_vsync_enabled();
            mDebugOverlay->notify(sq::build_string("vsync set to ", STRINGS[newValue]));
            mWindow->set_vsync_enabled(newValue);
            refresh_options();
        }

        if (data.keyboard.key == Key::B)
        {
            constexpr const auto STRINGS = std::array { "OFF", "ON" };
            mOptions->bloom_enable = !mOptions->bloom_enable;
            mDebugOverlay->notify(sq::build_string("bloom set to ", STRINGS[mOptions->bloom_enable]));
            refresh_options();
        }

        if (data.keyboard.key == Key::O)
        {
            constexpr const auto STRINGS = std::array { "OFF", "LOW", "HIGH" };
            if (++mOptions->ssao_quality == 3) mOptions->ssao_quality = 0;
            mDebugOverlay->notify(sq::build_string("ssao set to ", STRINGS[mOptions->ssao_quality]));
            refresh_options();
        }

        if (data.keyboard.key == Key::A)
        {
            mOptions->msaa_quality = mOptions->msaa_quality * 2;
            if (mOptions->msaa_quality == 16)
            {
                mDebugOverlay->notify("msaa set to OFF");
                mOptions->msaa_quality = 1;
            }
            else mDebugOverlay->notify("msaa set to {}x"_format(mOptions->msaa_quality));
            refresh_options();
        }

        if (data.keyboard.key == Key::D)
        {
            if      (mOptions->debug_texture == "")      mOptions->debug_texture = "depth";
            else if (mOptions->debug_texture == "depth") mOptions->debug_texture = "bloom";
            else if (mOptions->debug_texture == "bloom") mOptions->debug_texture = "ssao";
            else if (mOptions->debug_texture == "ssao")  mOptions->debug_texture = "";

            mDebugOverlay->notify("debug texture set to '{}'"_format(mOptions->debug_texture));
            refresh_options();
        }

        if (data.keyboard.key == Key::I)
        {
            constexpr const auto STRINGS = std::array { "OFF", "ON" };
            mOptions->imgui_demo = !mOptions->imgui_demo;
            mDebugOverlay->notify(sq::build_string("imgui demo set to ", STRINGS[mOptions->imgui_demo]));
        }

        if (data.keyboard.key == Key::R)
        {
            constexpr const auto STRINGS = std::array { "OFF", "ON" };
            mOptions->render_hit_blobs = mOptions->render_hurt_blobs = !mOptions->render_skeletons;
            mOptions->render_diamonds = mOptions->render_skeletons = !mOptions->render_skeletons;
            mDebugOverlay->notify(sq::build_string("debug render set to ", STRINGS[mOptions->render_skeletons]));
        }

        if (data.keyboard.key == Key::Num_1)
        {
            constexpr const auto STRINGS = std::array { "false", "true" };
            mOptions->debug_toggle_1 = !mOptions->debug_toggle_1;
            mDebugOverlay->notify(sq::build_string("debugToggle1 set to ", STRINGS[mOptions->debug_toggle_1]));
            refresh_options();
        }

        if (data.keyboard.key == Key::Num_2)
        {
            constexpr const auto STRINGS = std::array { "false", "true" };
            mOptions->debug_toggle_2 = !mOptions->debug_toggle_2;
            mDebugOverlay->notify(sq::build_string("debugToggle2 set to ", STRINGS[mOptions->debug_toggle_2]));
            refresh_options();
        }
    }

    //--------------------------------------------------------//

    mActiveScene->handle_event(event);
}

//============================================================================//

void SmashApp::refresh_options()
{
    const auto& ctx = sq::VulkanContext::get();

    mWindow->destroy_swapchain_and_friends();
    mActiveScene->refresh_options_destroy();

    ctx.allocator.free_empty_blocks();

    mWindow->create_swapchain_and_friends();
    mActiveScene->refresh_options_create();

    mResourceCaches->refresh_options();
}

//============================================================================//

void SmashApp::start_game(GameSetup setup)
{
    mActiveScene = std::make_unique<GameScene>(*this, setup);
    refresh_options();
}

void SmashApp::start_action_editor()
{
    mActiveScene = std::make_unique<EditorScene>(*this);
    refresh_options();
}

void SmashApp::return_to_main_menu()
{
    mActiveScene = std::make_unique<MenuScene>(*this);
    mWindow->set_title("SuperTuxSmash - Main Menu");
    refresh_options();
}
