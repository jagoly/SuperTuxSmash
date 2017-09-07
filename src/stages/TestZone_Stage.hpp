#pragma once

#include "game/Stage.hpp"

//============================================================================//

namespace sts {

class TestZone_Stage final : public Stage
{
public: //====================================================//

    TestZone_Stage(FightWorld& world);

    //--------------------------------------------------------//

    void tick() override;
};

} // namespace sts
