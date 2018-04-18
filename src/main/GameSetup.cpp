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
