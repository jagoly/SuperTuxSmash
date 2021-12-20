#include "main/GameSetup.hpp"

using namespace sts;

//============================================================================//

GameSetup GameSetup::get_defaults()
{
    GameSetup result;

    result.players.push_back({FighterEnum::Sara});
    result.players.push_back({FighterEnum::Tux});

    result.stage = StageEnum::TestZone;

    return result;
}

//============================================================================//

GameSetup GameSetup::get_quickstart()
{
    // change this to whatever I need for testing

    GameSetup result;

    result.players.push_back({FighterEnum::Mario});
    result.players.push_back({FighterEnum::Mario});
    result.players.push_back({FighterEnum::Mario});
    result.players.push_back({FighterEnum::Mario});

    result.stage = StageEnum::TestZone;

    return result;
}
