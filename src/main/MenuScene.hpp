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

    void refresh_options_destroy() override;

    void refresh_options_create() override;

    void show_imgui_widgets() override;

    void populate_command_buffer(vk::CommandBuffer cmdbuf, vk::Framebuffer framebuf) override;

private: //===================================================//

    void update() override;

    void integrate(double elapsed, float blend) override;

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    std::vector<TinyString> mStageNames;
    std::vector<TinyString> mFighterNames;

    GameSetup mSetup;
};

//============================================================================//

} // namespace sts
