#pragma once

#include <sqee/app/Event.hpp>
#include <sqee/app/GuiSystem.hpp>
#include <sqee/app/Scene.hpp>

#include "render/Renderer.hpp"

#include "game/FightWorld.hpp"
#include "game/Stage.hpp"
#include "game/Controller.hpp"

#include "main/Options.hpp"
#include "main/SmashApp.hpp"

//============================================================================//

namespace sts {

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

    UniquePtr<FightWorld> mFightWorld;

    UniquePtr<Renderer> mRenderer;

    Array<UniquePtr<Controller>, 4> mControllers;

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    void impl_show_general_window();
    void impl_show_fighters_window();

    //--------------------------------------------------------//

    bool mGamePaused = false;
};

} // namespace sts
