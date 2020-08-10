#include "main/GameSetup.hpp"

using namespace sts;

//============================================================================//

GameSetup GameSetup::get_defaults()
{
    GameSetup result;

    result.players[0].enabled = true;
    result.players[0].fighter = FighterEnum::Sara;

    result.players[1].enabled = true;
    result.players[1].fighter = FighterEnum::Tux;

    result.stage = StageEnum::TestZone;

    return result;
}

//============================================================================//

GameSetup GameSetup::get_quickstart()
{
    // change this to whatever I need for testing

    GameSetup result;

    result.players[0].enabled = true;
    result.players[0].fighter = FighterEnum::Mario;

    result.players[1].enabled = true;
    result.players[1].fighter = FighterEnum::Mario;

    result.stage = StageEnum::TestZone;

    return result;
}
