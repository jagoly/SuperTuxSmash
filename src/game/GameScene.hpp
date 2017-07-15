#pragma once

#include <sqee/app/Event.hpp>
#include <sqee/app/Scene.hpp>

#include "render/Renderer.hpp"

#include "game/Stage.hpp"
#include "game/Controller.hpp"
#include "game/Fighter.hpp"

#include "main/Options.hpp"

//============================================================================//

namespace sts {

class GameScene final : public sq::Scene
{
public: //====================================================//

    GameScene(const sq::InputDevices& inputDevices, const Options& options);

    ~GameScene() override;

    //--------------------------------------------------------//

    void refresh_options();

    void handle_event(sq::Event event);

public: //====================================================//

    void update() override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    unique_ptr<Renderer> mRenderer;

    unique_ptr<Stage> mStage;

    std::array<unique_ptr<Controller>, 4> mControllers;
    std::array<unique_ptr<Fighter>, 4> mFighters;

    //--------------------------------------------------------//

    const sq::InputDevices& mInputDevices;

    const Options& mOptions;
};

} // namespace sts
