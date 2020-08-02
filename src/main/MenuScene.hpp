#pragma once

#include "setup.hpp"

#include "main/GameSetup.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/app/Scene.hpp>

namespace sts {

//============================================================================//

class MenuScene final : public sq::Scene
{
public: //====================================================//

    MenuScene(SmashApp& smashApp);

    ~MenuScene() override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event) override;

    void refresh_options() override;

    void show_imgui_widgets() override;

private: //===================================================//

    void update() override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    GameSetup mSetup;
};

//============================================================================//

} // namespace sts
