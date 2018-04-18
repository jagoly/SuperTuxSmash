#pragma once

#include <sqee/builtins.hpp>

#include "enumerations.hpp"

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

    std::array<Player, 4> players;

    StageEnum stage {-1};

    //--------------------------------------------------------//

    static GameSetup get_defaults();
};

} // namespace sts
