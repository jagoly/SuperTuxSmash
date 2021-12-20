#pragma once

#include "setup.hpp"

#include "main/MainEnums.hpp"

namespace sts {

//============================================================================//

struct GameSetup
{
    struct Player
    {
        FighterEnum fighter {-1};
    };

    StackVector<Player, MAX_FIGHTERS> players;

    StageEnum stage {-1};

    static GameSetup get_defaults();

    static GameSetup get_quickstart();
};

//============================================================================//

} // namespace sts
