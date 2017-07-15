#pragma once

#include "game/Fighter.hpp"

//============================================================================//

namespace sts {

class Cheese_Fighter final : public Fighter
{
public: //====================================================//

    Cheese_Fighter(Controller& controller);

    //--------------------------------------------------------//

    void tick() override;

private: //===================================================//

    Vec3F colour;

    //--------------------------------------------------------//

    friend class Cheese_Render;
};

} // namespace sts
