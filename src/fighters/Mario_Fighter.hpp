#pragma once

#include "game/Fighter.hpp"

//============================================================================//

namespace sts {

class Mario_Fighter final : public Fighter
{
public: //====================================================//

    Mario_Fighter(uint8_t index, FightWorld& world);

    //--------------------------------------------------------//

    void tick() override;

private: //===================================================//

    friend class Mario_Render;
};

} // namespace sts
