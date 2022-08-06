#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

struct GameSetup
{
    struct Player
    {
        TinyString fighter;
    };

    StackVector<Player, MAX_FIGHTERS> players;

    TinyString stage;

    static GameSetup get_defaults();

    static GameSetup get_quickstart();
};

//============================================================================//

} // namespace sts
