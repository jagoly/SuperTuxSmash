#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>

#include <sqee/gl/Context.hpp>

#include <main/SmashApp.hpp>
#include <render/Renderer.hpp>

#include <game/stages/TestZone/Stage.hpp>
#include <game/fighters/Cheese/Fighter.hpp>
#include <game/fighters/Sara/Fighter.hpp>

#include <render/stages/TestZone.hpp>
#include <render/fighters/Cheese.hpp>
#include <render/fighters/Sara.hpp>

#include "GameScene.hpp"

using namespace sts;
namespace maths = sq::maths;

//============================================================================//

GameScene::GameScene(SmashApp& smashApp)
    : sq::Scene(1.0 / 48.0), mSmashApp(smashApp)
{
    mRenderer = std::make_unique<Renderer>();

    mStage = std::make_unique<stages::TestZone>();

    mFighters.push_back(std::make_unique<fighters::Cheese_Fighter>(*mStage));
    mFighters.push_back(std::make_unique<fighters::Sara_Fighter>(*mStage));

    auto& fighterA = static_cast<fighters::Cheese_Fighter&>(*mFighters[0]);
    auto& fighterB = static_cast<fighters::Sara_Fighter&>(*mFighters[1]);

    mRenderer->add_fighter(std::make_unique<fighters::Cheese_Render>(*mRenderer, fighterA));
    mRenderer->add_fighter(std::make_unique<fighters::Sara_Render>(*mRenderer, fighterB));

    fighterA.mController.load_config("player1.txt");
    fighterB.mController.load_config("player2.txt");
}

//============================================================================//

void GameScene::update_options()
{
    mRenderer->update_options();
}

//============================================================================//

bool GameScene::handle(sf::Event event)
{
    const auto& fighterA = mFighters[0];
    const auto& fighterB = mFighters[1];

    //========================================================//

    if (fighterA->mController.handle_event(event)) return true;
    if (fighterB->mController.handle_event(event)) return true;

//    if (event.type == sf::Event::KeyPressed)
//    {
//        if (event.key.code == sf::Keyboard::Space)
//        {
//            fighterA->input_press(Fighter::InputPress::Jump);
//            return true;
//        }

//        if (event.key.code == sf::Keyboard::Z)
//        {
//            fighterA->input_press(Fighter::InputPress::Attack);
//            return true;
//        }

//        if (event.key.code == sf::Keyboard::Left)
//        {
//            fighterA->input_press(Fighter::InputPress::Left);
//            return true;
//        }

//        if (event.key.code == sf::Keyboard::Right)
//        {
//            fighterA->input_press(Fighter::InputPress::Right);
//            return true;
//        }

//        if (event.key.code == sf::Keyboard::Down)
//        {
//            fighterA->input_press(Fighter::InputPress::Down);
//            return true;
//        }

//        if (event.key.code == sf::Keyboard::Up)
//        {
//            fighterA->input_press(Fighter::InputPress::Up);
//            return true;
//        }
//    }

    //========================================================//

    return false;
}

//============================================================================//

void GameScene::tick()
{
    const auto& fighterA = mFighters[0];

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
    const float progress = float(accumulation) * 48.f;

    mRenderer->render(progress);
}
