#pragma once

#include <sqee/app/Application.hpp>
#include <sqee/app/Window.hpp>
#include <sqee/app/InputDevices.hpp>
#include <sqee/app/DebugOverlay.hpp>

#include "main/Options.hpp"
#include "main/GameSetup.hpp"

//============================================================================//

namespace sts {

class SmashApp final : public sq::Application
{
public: //====================================================//

    SmashApp();

    ~SmashApp() override;

    //--------------------------------------------------------//

    void start_game(GameSetup setup);

    void return_to_main_menu();

    //--------------------------------------------------------//

    sq::Window& get_window() { return *mWindow; }
    sq::InputDevices& get_input_devices() { return *mInputDevices; }
    sq::DebugOverlay& get_debug_overlay() { return *mDebugOverlay; }

    Options& get_options() { return mOptions; }

private: //===================================================//

    void initialise(std::vector<string> args) override;

    void update(double elapsed) override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event);

    void refresh_options();

    //--------------------------------------------------------//

    unique_ptr<sq::Window> mWindow;
    unique_ptr<sq::InputDevices> mInputDevices;
    unique_ptr<sq::DebugOverlay> mDebugOverlay;

    Options mOptions;

    //--------------------------------------------------------//

    unique_ptr<sq::Scene> mActiveScene;
};

} // namespace sts
