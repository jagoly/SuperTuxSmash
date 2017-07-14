#pragma once

#include <sqee/app/Event.hpp>
#include <sqee/app/Scene.hpp>

#include "game/Renderer.hpp"
#include "game/Stage.hpp"
#include "game/Fighter.hpp"

#include "main/Options.hpp"

//============================================================================//

namespace sts {

class GameScene final : public sq::Scene
{
public: //====================================================//

    GameScene(const sq::InputDevices& inputDevices, const Options& options);

    //--------------------------------------------------------//

    void refresh_options();

    void handle_event(sq::Event event);

public: //====================================================//

    void update() override;

    void render(double elapsed) override;

    //--------------------------------------------------------//

    unique_ptr<Game> mGame;

    const sq::InputDevices& mInputDevices;

    const Options& mOptions;
};

} // namespace sts
