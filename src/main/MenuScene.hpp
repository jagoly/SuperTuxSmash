#pragma once

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiSystem.hpp>
#include <sqee/app/Scene.hpp>

#include "main/Options.hpp"
#include "main/SmashApp.hpp"

//============================================================================//

namespace sts {

class MenuScene final : public sq::Scene
{
public: //====================================================//

    MenuScene(SmashApp& smashApp);

    ~MenuScene() override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event) override;

    void refresh_options() override;

private: //===================================================//

    void update() override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    sq::GuiWidget mMainWidget;

    void impl_show_main_window();

    //--------------------------------------------------------//

    struct {

        struct {
            FighterEnum fighter {-1};
        } players[4];

        StageEnum stage {-1};

    } state;

    GameSetup setup;
};

} // namespace sts
