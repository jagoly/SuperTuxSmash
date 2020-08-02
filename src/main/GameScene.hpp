#pragma once

#include "setup.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/app/Scene.hpp>

namespace sts {

//============================================================================//

class GameScene final : public sq::Scene
{
public: //====================================================//

    GameScene(SmashApp& smashApp, GameSetup setup);

    ~GameScene() override;

    //--------------------------------------------------------//

    void handle_event(sq::Event event) override;

    void refresh_options() override;

    void show_imgui_widgets() override;

private: //===================================================//

    void update() override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    std::unique_ptr<FightWorld> mFightWorld;

    std::unique_ptr<Renderer> mRenderer;

    std::array<std::unique_ptr<Controller>, 4> mControllers;

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    void impl_show_general_window();
    void impl_show_fighters_window();

    //--------------------------------------------------------//

    bool mGamePaused = false;
};

//============================================================================//

} // namespace sts
