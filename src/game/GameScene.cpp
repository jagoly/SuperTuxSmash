#include "game/Game.hpp"
#include "game/Renderer.hpp"
#include "game/Stage.hpp"
#include "game/Controller.hpp"
#include "game/Fighter.hpp"

#include "game/fighters/Sara/Fighter.hpp"
#include "game/fighters/Cheese/Fighter.hpp"

#include "GameScene.hpp"

using namespace sts;

//============================================================================//

GameScene::GameScene(const sq::InputDevices& inputDevices, const Options& options)
    : Scene(1.0 / 48.0), mInputDevices(inputDevices), mOptions(options)
{
    mGame = std::make_unique<Game>(mInputDevices, mOptions);
}

//============================================================================//

void GameScene::refresh_options()
{
    mGame->renderer->refresh_options();
}

void GameScene::handle_event(sq::Event event)
{
    for (auto& controller : mGame->controllers)
    {
        if (controller != nullptr)
            controller->handle_event(event);
    }
}

//============================================================================//

void GameScene::update()
{
    //mStage->tick();

    for (auto& fighter : mGame->fighters)
    {
        if (fighter != nullptr)
            fighter->tick();
    }
}

//============================================================================//

void GameScene::render(double)
{
    mGame->renderer->render(float(mAccumulation / mTickTime));
}
