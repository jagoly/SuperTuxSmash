#include <sqee/app/GuiSystem.hpp>

#include <sqee/debug/Logging.hpp>
#include <sqee/debug/Misc.hpp>

#include "main/MenuScene.hpp"
#include "main/GameScene.hpp"

#include "editor/ActionEditor.hpp"

#include "SmashApp.hpp"

using namespace sts;
namespace maths = sq::maths;

//============================================================================//

SmashApp::SmashApp() = default;

SmashApp::~SmashApp() = default;

//============================================================================//

void SmashApp::initialise(Vector<String> args)
{
    mWindow = std::make_unique<sq::Window>("SuperTuxSmash", Vec2U(1280u, 720u));

    mWindow->set_vsync_enabled(true);
    mWindow->set_key_repeat(false);

    mInputDevices = std::make_unique<sq::InputDevices>(*mWindow);

    sq::GuiSystem::construct(*mWindow, *mInputDevices);

    mDebugOverlay = std::make_unique<sq::DebugOverlay>();

    //--------------------------------------------------------//

    return_to_main_menu();
}

//============================================================================//

void SmashApp::update(double elapsed)
{
    auto& guiSystem = sq::GuiSystem::get();

    //-- fetch and handle events -----------------------------//

    for (auto event : mWindow->fetch_events())
    {
        if (guiSystem.handle_event(event)) continue;
        handle_event(event);
    }

    guiSystem.finish_handle_events();

    //-- update and render the active scene ------------------//

    if (mActiveScene != nullptr)
        mActiveScene->update_and_render(elapsed);

    //-- update and render the debug overlay -----------------//

    mDebugOverlay->update_and_render(elapsed);

    //-- draw and render imgui stuff -------------------------//

    guiSystem.show_imgui_demo();

    guiSystem.draw_widgets();

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

    //--------------------------------------------------------//

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

    auto notify = [this](uint value, const String& message, Vector<String> options)
    { this->mDebugOverlay->notify(message + options[value]); };

    if (type == Type::Keyboard_Press)
    {
        if (data.keyboard.key == Key::V)
        {
            auto value = !mWindow->get_vsync_enabled();
            mWindow->set_vsync_enabled(value);
            notify(value, "vsync set to ", {"OFF", "ON"});
        }

        if (data.keyboard.key == Key::B)
        {
            mOptions.Bloom_Enable = !mOptions.Bloom_Enable;
            notify(mOptions.Bloom_Enable, "bloom set to ", {"OFF", "ON"});
            refresh_options();
        }

        if (data.keyboard.key == Key::O)
        {
            if (++mOptions.SSAO_Quality == 3) mOptions.SSAO_Quality = 0;
            notify(mOptions.SSAO_Quality, "ssao set to ", {"OFF", "LOW", "HIGH"});
            refresh_options();
        }

        if (data.keyboard.key == Key::A)
        {
            if (++mOptions.MSAA_Quality == 3) mOptions.MSAA_Quality = 0;
            notify(mOptions.MSAA_Quality, "msaa set to ", {"OFF", "4x", "16x"});
            refresh_options();
        }

        if (data.keyboard.key == Key::D)
        {
            if      ( mOptions.Debug_Texture == ""      ) mOptions.Debug_Texture = "depth";
            else if ( mOptions.Debug_Texture == "depth" ) mOptions.Debug_Texture = "ssao";
            else if ( mOptions.Debug_Texture == "ssao"  ) mOptions.Debug_Texture = "bloom";
            else if ( mOptions.Debug_Texture == "bloom" ) mOptions.Debug_Texture = "";

            mDebugOverlay->notify("debug texture set to \"" + mOptions.Debug_Texture + "\"");

            refresh_options();
        }

        #ifdef SQEE_DEBUG

        if (data.keyboard.key == Key::Num_1)
        {
            sqeeDebugToggle1 = !sqeeDebugToggle1;
            notify(sqeeDebugToggle1, "sqeeDebugToggle1 set to ", {"false", "true"});
            refresh_options();
        }

        if (data.keyboard.key == Key::Num_2)
        {
            sqeeDebugToggle2 = !sqeeDebugToggle2;
            notify(sqeeDebugToggle2, "sqeeDebugToggle2 set to ", {"false", "true"});
            refresh_options();
        }

        #endif
    }

    //--------------------------------------------------------//

    if (mActiveScene != nullptr)
        mActiveScene->handle_event(event);
}

//============================================================================//

void SmashApp::refresh_options()
{
    mOptions.Window_Size = mWindow->get_window_size();

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
    mActiveScene = std::make_unique<ActionEditor>(*this);
    refresh_options();
}

void SmashApp::return_to_main_menu()
{
    mActiveScene = std::make_unique<MenuScene>(*this);
    refresh_options();
}
