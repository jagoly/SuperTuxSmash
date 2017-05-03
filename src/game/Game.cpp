#include <game/stages/TestZone/Stage.hpp>
#include <game/fighters/Cheese/Fighter.hpp>
#include <game/fighters/Sara/Fighter.hpp>

#include "Game.hpp"

using namespace sts;

//============================================================================//

Game::Game()
{
    renderer = std::make_unique<Renderer>(*this);

    stage = std::make_unique<stages::TestZone>(*this);

    fighters.push_back(std::make_unique<fighters::Cheese_Fighter>(*this));
    fighters.push_back(std::make_unique<fighters::Sara_Fighter>(*this));

    fighters[0]->controller->load_config("player1.txt");
    fighters[1]->controller->load_config("player2.txt");

    fighters[0]->setup();
    fighters[1]->setup();
}
