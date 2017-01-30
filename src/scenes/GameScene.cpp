#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>

#include <sqee/gl/Context.hpp>

#include <main/SmashApp.hpp>
#include <game/Renderer.hpp>

#include <game/stages/TestZone/Stage.hpp>
#include <game/fighters/Cheese/Fighter.hpp>
#include <game/fighters/Sara/Fighter.hpp>

#include "GameScene.hpp"

using namespace sts;
namespace maths = sq::maths;

//============================================================================//

GameScene::GameScene(SmashApp& smashApp)
    : sq::Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    mRenderer = std::make_unique<Renderer>();

    mStage = std::make_unique<stages::TestZone>();

    mControllers.push_back(std::make_unique<Controller>());
    mControllers.push_back(std::make_unique<Controller>());

    mFighters.push_back(std::make_unique<fighters::Cheese_Fighter>());
    mFighters.push_back(std::make_unique<fighters::Sara_Fighter>());

    mFighters[0]->mController = mControllers[0].get();
    mFighters[0]->mRenderer = mRenderer.get();
    mFighters[0]->setup();

    mFighters[1]->mController = mControllers[1].get();
    mFighters[1]->mRenderer = mRenderer.get();
    mFighters[1]->setup();

    mControllers[0]->load_config("player1.txt");
    mControllers[1]->load_config("player2.txt");

    mRenderer->functions.draw_FighterA = [&]() { mFighters[0]->render(); };
    mRenderer->functions.draw_FighterB = [&]() { mFighters[1]->render(); };
}

//============================================================================//

void GameScene::update_options()
{
    mRenderer->update_options();
}

//============================================================================//

bool GameScene::handle(sf::Event event)
{
    if (mControllers[0]->handle_event(event)) return true;
    if (mControllers[1]->handle_event(event)) return true;

    return false;
}

//============================================================================//

void GameScene::tick()
{
    //========================================================//

//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
//        fighterA->input_hold(Fighter::InputHold::Jump);

//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
//        fighterA->input_hold(Fighter::InputHold::Attack);

//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
//        fighterA->input_hold(Fighter::InputHold::Left);

//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
//        fighterA->input_hold(Fighter::InputHold::Right);

//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
//        fighterA->input_hold(Fighter::InputHold::Down);

//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
//        fighterA->input_hold(Fighter::InputHold::Up);

    //========================================================//

    mStage->tick();

    for (auto& fighter : mFighters)
    {
        fighter->tick();
    }
}

//============================================================================//

void GameScene::render()
{
    const float progress = float(accumulation / tickTime);

    mRenderer->progress = progress;

    mRenderer->render();
}
