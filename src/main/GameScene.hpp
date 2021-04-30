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

    void refresh_options_destroy() override;

    void refresh_options_create() override;

    void show_imgui_widgets() override;

    void populate_command_buffer(vk::CommandBuffer cmdbuf, vk::Framebuffer framebuf) override;

private: //===================================================//

    void update() override;

    void integrate(double elapsed, float blend) override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    std::unique_ptr<Renderer> mRenderer;

    std::unique_ptr<FightWorld> mFightWorld;

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
