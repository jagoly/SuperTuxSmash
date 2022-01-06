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

    //--------------------------------------------------------//

    std::unique_ptr<Renderer> mRenderer;

    std::unique_ptr<World> mWorld;

    sq::StackVector<std::unique_ptr<Controller>, MAX_FIGHTERS> mControllers;

    std::unique_ptr<StandardCamera> mStandardCamera;

    std::unique_ptr<EditorCamera> mEditorCamera;

    //--------------------------------------------------------//

    SmashApp& mSmashApp;

    //--------------------------------------------------------//

    void impl_show_general_window();
    void impl_show_objects_window();

    //--------------------------------------------------------//

    bool mGamePaused = false;
};

//============================================================================//

} // namespace sts
