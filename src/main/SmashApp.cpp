#include <sqee/debug/Logging.hpp>
#include <sqee/debug/Misc.hpp>

#include "SmashApp.hpp"

using namespace sts;
namespace maths = sq::maths;

//============================================================================//

SmashApp::SmashApp() = default;

SmashApp::~SmashApp() = default;

//============================================================================//

void SmashApp::initialise(std::vector<string> args)
{
    mWindow = std::make_unique<sq::Window>("SuperTuxSmash", Vec2U(1280u, 720u));

    mInputDevices = std::make_unique<sq::InputDevices>(*mWindow);

    mDebugOverlay = std::make_unique<sq::DebugOverlay>();

    mGameScene = std::make_unique<GameScene>(*mInputDevices, mOptions);
}

//============================================================================//

void SmashApp::update(double elapsed)
{
    //-- fetch and handle events -----------------------------//

    for (auto event : mWindow->fetch_events())
        handle_event(event);

    //-- update and render the game scene --------------------//

    if (mGameScene != nullptr)
        mGameScene->update_and_render(elapsed);

    //-- update and render the debug overlay -----------------//

    mDebugOverlay->update_and_render(elapsed);

    //-- drawing is done -------------------------------------//

    mWindow->swap_buffers();
}

//============================================================================//

void SmashApp::refresh_options()
{
    mOptions.Window_Size = mWindow->get_window_size();

    if (mGameScene != nullptr) mGameScene->refresh_options();
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

    if (type == Type::Keyboard_Press && data.keyboard.key == Key::Menu)
    {
        mDebugOverlay->toggle_active();
        return;
    }

    //--------------------------------------------------------//

    auto notify = [this](uint value, const string& message, std::vector<string> options)
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
            mOptions.SSAO_Quality = ++mOptions.SSAO_Quality == 3 ? 0 : mOptions.SSAO_Quality;
            notify(mOptions.SSAO_Quality, "ssao set to ", {"OFF", "LOW", "HIGH"});
            refresh_options();
        }

        if (data.keyboard.key == Key::D)
        {
            if      ( mOptions.Debug_Texture == ""           ) mOptions.Debug_Texture = "diffuse";
            else if ( mOptions.Debug_Texture == "diffuse"    ) mOptions.Debug_Texture = "surface";
            else if ( mOptions.Debug_Texture == "surface"    ) mOptions.Debug_Texture = "normals";
            else if ( mOptions.Debug_Texture == "normals"    ) mOptions.Debug_Texture = "specular";
            else if ( mOptions.Debug_Texture == "specular"   ) mOptions.Debug_Texture = "lighting";
            else if ( mOptions.Debug_Texture == "lighting"   ) mOptions.Debug_Texture = "ssao";
            else if ( mOptions.Debug_Texture == "ssao"       ) mOptions.Debug_Texture = "bloom";
            else if ( mOptions.Debug_Texture == "bloom"      ) mOptions.Debug_Texture = "";

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

    mGameScene->handle_event(event);
}
