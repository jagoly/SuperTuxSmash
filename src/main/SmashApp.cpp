#include "main/SmashApp.hpp"

#include "main/Options.hpp"
#include "main/Resources.hpp"

#include "editor/EditorScene.hpp"
#include "main/GameScene.hpp"
#include "main/MenuScene.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiSystem.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>

using namespace sts;

//============================================================================//

SmashApp::SmashApp() = default;

SmashApp::~SmashApp() = default;

//============================================================================//

void SmashApp::initialise(std::vector<String> /*args*/)
{
    mWindow = std::make_unique<sq::Window>("SuperTuxSmash - Main Menu", Vec2U(1280u, 720u));

    mWindow->set_vsync_enabled(true);
    mWindow->set_key_repeat(false);

    mInputDevices = std::make_unique<sq::InputDevices>(*mWindow);
    mDebugOverlay = std::make_unique<sq::DebugOverlay>();
    mAudioContext = std::make_unique<sq::AudioContext>();
    mPreProcessor = std::make_unique<sq::PreProcessor>();

    mOptions = std::make_unique<Options>();

    mResourceCaches = std::make_unique<ResourceCaches>(*mAudioContext, *mPreProcessor);

    sq::GuiSystem::construct(*mWindow, *mInputDevices);
    sq::GuiSystem::get().set_style_widgets_supertux();
    sq::GuiSystem::get().set_style_colours_supertux();

    return_to_main_menu();
}

//============================================================================//

void SmashApp::update(double elapsed)
{
    auto& guiSystem = sq::GuiSystem::get();

    SQASSERT(mActiveScene != nullptr, "we have no scene!");

    //-- fetch and handle events -----------------------------//

    if (mWindow->has_focus() == true)
    {
        for (const auto& event : mWindow->fetch_events())
        {
            if (guiSystem.handle_event(event)) continue;
            handle_event(event);
        }
        guiSystem.finish_handle_events(true);
    }
    else guiSystem.finish_handle_events(false);

    //-- update and render scenes ----------------------------//

    mActiveScene->update_and_render(elapsed);
    mDebugOverlay->update_and_render(elapsed);

    //-- draw and render imgui stuff -------------------------//

    mActiveScene->show_imgui_widgets();

    if (mOptions->imgui_demo == true)
        guiSystem.show_imgui_demo();

    guiSystem.finish_scene_update(elapsed);

    guiSystem.render_gui();

    //-- drawing is done -------------------------------------//

    mWindow->swap_buffers();
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
            constexpr const auto STRINGS = std::array { "OFF", "4x", "16x" };
            if (++mOptions->msaa_quality == 3) mOptions->msaa_quality = 0;
            mDebugOverlay->notify(sq::build_string("msaa set to ", STRINGS[mOptions->msaa_quality]));
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
    mOptions->window_size = mWindow->get_window_size();

    if (mActiveScene != nullptr)
        mActiveScene->refresh_options();
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
    mWindow->set_window_title("SuperTuxSmash - Main Menu");
    refresh_options();
}
