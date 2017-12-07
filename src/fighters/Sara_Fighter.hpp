#pragma once

#include "game/ParticleSet.hpp"

#include "game/Fighter.hpp"

//============================================================================//

namespace sts {

class Sara_Fighter final : public Fighter
{
public: //====================================================//

    Sara_Fighter(uint8_t index, FightWorld& world);

    //--------------------------------------------------------//

    void tick() override;

private: //===================================================//

    ParticleSet mParticleSet;

    friend class Sara_Render;
};

} // namespace sts
