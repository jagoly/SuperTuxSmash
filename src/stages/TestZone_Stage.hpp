#pragma once

#include "game/Stage.hpp"

namespace sts {

class TestZone_Render;

//============================================================================//

class TestZone_Stage final : public Stage
{
public: //====================================================//

    TestZone_Stage(FightWorld& world);

    //--------------------------------------------------------//

    void tick() override;

private: //===================================================//

    friend TestZone_Render;
};

//============================================================================//

} // namespace sts
