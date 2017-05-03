#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>

#include <main/SmashApp.hpp>

#include <game/Game.hpp>
#include <game/Renderer.hpp>
#include <game/Stage.hpp>
#include <game/Controller.hpp>
#include <game/Fighter.hpp>

#include "GameScene.hpp"

using namespace sts;
namespace maths = sq::maths;

//============================================================================//

GameScene::GameScene(SmashApp& smashApp)
    : sq::Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    mGame = std::make_unique<Game>();
}

//============================================================================//

void GameScene::update_options()
{
    mGame->renderer->update_options();
}

//============================================================================//

bool GameScene::handle(sf::Event event)
{
    for (auto& fighter : mGame->fighters)
        if (fighter->controller->handle_event(event))
            return true;

    return false;
}

//============================================================================//

void GameScene::tick()
{
    mGame->stage->tick();

    for (auto& fighter : mGame->fighters)
        fighter->tick();
}

//============================================================================//

void GameScene::render()
{
    mGame->renderer->progress = float(accumulation / tickTime);

    mGame->renderer->render();
}
