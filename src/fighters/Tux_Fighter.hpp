#pragma once

#include "game/Fighter.hpp"

//============================================================================//

namespace sts {

class Tux_Fighter final : public Fighter
{
public: //====================================================//

    Tux_Fighter(uint8_t index, FightWorld& world, Controller& controller);

    //--------------------------------------------------------//

    void tick() override;

private: //===================================================//

    friend class Tux_Render;
};

} // namespace sts
