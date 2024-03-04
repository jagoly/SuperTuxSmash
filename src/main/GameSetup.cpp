#include "main/GameSetup.hpp"

using namespace sts;

//============================================================================//

GameSetup GameSetup::get_quickstart()
{
    // change this to whatever I need for testing

    GameSetup result;

    result.players.push_back({"Mario"});
    result.players.push_back({"Mario"});
    result.players.push_back({"Mario"});
    result.players.push_back({"Mario"});

    result.stage = "TestZone";

    return result;
}
