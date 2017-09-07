#pragma once

#include "game/Fighter.hpp"

//============================================================================//

namespace sts {

class Sara_Fighter final : public Fighter
{
public: //====================================================//

    Sara_Fighter(uint8_t index, FightWorld& world, Controller& controller);

    //--------------------------------------------------------//

    void tick() override;

private: //===================================================//

    friend class Sara_Render;
};

} // namespace sts
