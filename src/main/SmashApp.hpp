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

    void start_action_editor();

    void return_to_main_menu();

    //--------------------------------------------------------//

    sq::Window& get_window() { return *mWindow; }
    sq::InputDevices& get_input_devices() { return *mInputDevices; }
    sq::DebugOverlay& get_debug_overlay() { return *mDebugOverlay; }

    Options& get_options() { return mOptions; }

private: //===================================================//

    void initialise(Vector<String> args) override;

    void update(double elapsed) override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event);

    void refresh_options();

    //--------------------------------------------------------//

    UniquePtr<sq::Window> mWindow;
    UniquePtr<sq::InputDevices> mInputDevices;
    UniquePtr<sq::DebugOverlay> mDebugOverlay;

    Options mOptions;

    //--------------------------------------------------------//

    UniquePtr<sq::Scene> mActiveScene;
};

} // namespace sts
