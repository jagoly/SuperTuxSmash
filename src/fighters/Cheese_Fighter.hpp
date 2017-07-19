#pragma once

#include <sqee/maths/Volumes.hpp>

#include "game/Fighter.hpp"

//============================================================================//

namespace sts {

class Cheese_Fighter final : public Fighter
{
public: //====================================================//

    Cheese_Fighter(uint8_t index, FightSystem& system, Controller& controller);

    //--------------------------------------------------------//

    void tick() override;

private: //===================================================//

    Vec3F mColour;

    std::vector<sq::maths::Sphere> mSpheres;

    //--------------------------------------------------------//

    friend class Cheese_Render;
};

} // namespace sts
