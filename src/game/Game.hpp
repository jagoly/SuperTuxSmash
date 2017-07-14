#pragma once

#include <sqee/builtins.hpp>
#include <sqee/app/InputDevices.hpp>

#include "main/Options.hpp"

//====== Forward Declarations ================================================//

namespace sts { class Renderer; class Stage; class Controller; class Fighter; }

//============================================================================//

namespace sts {

class Game final : sq::NonCopyable
{
public: //====================================================//

    Game(const sq::InputDevices& inputDevices, const Options& options);

    //--------------------------------------------------------//

    unique_ptr<Renderer> renderer;

    unique_ptr<Stage> stage;

    std::array<unique_ptr<Controller>, 4> controllers;
    std::array<unique_ptr<Fighter>, 4> fighters;

private: //===================================================//

    const sq::InputDevices& mInputDevices;

    const Options& mOptions;
};

} // namespace sts
