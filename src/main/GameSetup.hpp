#pragma once

#include <sqee/misc/Builtins.hpp>

#include "main/Enumerations.hpp"

//============================================================================//

namespace sts {

struct GameSetup
{
    //--------------------------------------------------------//

    struct Player
    {
        bool enabled = false;
        FighterEnum fighter {-1};
    };

    Array<Player, 4> players;

    StageEnum stage {-1};

    //--------------------------------------------------------//

    static GameSetup get_defaults();
};

} // namespace sts
