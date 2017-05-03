#pragma once

#include <sqee/builtins.hpp>

#include <game/Renderer.hpp>
#include <game/Stage.hpp>
#include <game/Fighter.hpp>

namespace sts {

//============================================================================//

class Game final
{
public:

    //========================================================//

    Game();

    //========================================================//

    unique_ptr<Renderer> renderer;

    unique_ptr<Stage> stage;

    vector<unique_ptr<Fighter>> fighters;
};

//============================================================================//

} // namespace sts
