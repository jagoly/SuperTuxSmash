#include "game/stages/TestZone/Stage.hpp"
#include "game/fighters/Cheese/Fighter.hpp"
#include "game/fighters/Sara/Fighter.hpp"

#include "game/Renderer.hpp"
#include "game/Game.hpp"

using namespace sts;

//============================================================================//

Game::Game(const sq::InputDevices& inputDevices, const Options& options)
    : mInputDevices(inputDevices), mOptions(options)
{
    renderer = std::make_unique<Renderer>(*this, options);

    stage = std::make_unique<stages::TestZone>(*this);

    controllers[0] = std::make_unique<Controller>(mInputDevices);
    controllers[1] = std::make_unique<Controller>(mInputDevices);

    controllers[0]->load_config("player1.txt");
    controllers[1]->load_config("player2.txt");

    fighters[0] = std::make_unique<fighters::Sara_Fighter>(*this, *controllers[0]);
    fighters[1] = std::make_unique<fighters::Cheese_Fighter>(*this, *controllers[1]);

    fighters[0]->setup();
    fighters[1]->setup();
}
