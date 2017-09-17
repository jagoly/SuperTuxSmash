#pragma once

#include <sqee/app/Application.hpp>
#include <sqee/app/Window.hpp>
#include <sqee/app/InputDevices.hpp>
#include <sqee/app/DebugOverlay.hpp>
#include <sqee/app/GuiSystem.hpp>

#include "main/Options.hpp"

#include "game/GameScene.hpp"

//============================================================================//

namespace sts {

class SmashApp final : public sq::Application
{
public: //====================================================//

    SmashApp();

    ~SmashApp();

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

    unique_ptr<sq::GuiSystem> mGuiSystem;

    //--------------------------------------------------------//

    Options mOptions;

    unique_ptr<GameScene> mGameScene;
};

} // namespace sts
