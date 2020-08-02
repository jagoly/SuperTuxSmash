#pragma once

#include "setup.hpp"

#include <sqee/app/Application.hpp>

#include <sqee/app/Window.hpp> // IWYU pragma: export
#include <sqee/app/InputDevices.hpp> // IWYU pragma: export
#include <sqee/app/DebugOverlay.hpp> // IWYU pragma: export
#include <sqee/app/Scene.hpp> // IWYU pragma: export

namespace sts {

//============================================================================//

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

    Options& get_options() { return *mOptions; }

private: //===================================================//

    void initialise(std::vector<String> args) override;

    void update(double elapsed) override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event);

    void refresh_options();

    //--------------------------------------------------------//

    std::unique_ptr<sq::Window> mWindow;
    std::unique_ptr<sq::InputDevices> mInputDevices;
    std::unique_ptr<sq::DebugOverlay> mDebugOverlay;

    std::unique_ptr<Options> mOptions;

    std::unique_ptr<sq::Scene> mActiveScene;
};

//============================================================================//

} // namespace sts
