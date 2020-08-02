#pragma once

#include "game/Fighter.hpp"

namespace sts {

class Sara_Render;

//============================================================================//

class Sara_Fighter final : public Fighter
{
public: //====================================================//

    Sara_Fighter(uint8_t index, FightWorld& world);

    //--------------------------------------------------------//

    void tick() override;

private: //===================================================//

    friend Sara_Render;
};

//============================================================================//

} // namespace sts
