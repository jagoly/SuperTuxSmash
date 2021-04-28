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

    void populate_command_buffer(vk::CommandBuffer cmdbuf, vk::Framebuffer framebuf) override;

private: //===================================================//

    void update() override;

    void integrate(double elapsed, float blend) override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    GameSetup mSetup;
};

//============================================================================//

} // namespace sts
