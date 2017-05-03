#pragma once

#include <game/Stage.hpp>

namespace sts { namespace stages {

//============================================================================//

class TestZone final : public sts::Stage
{
public:

    TestZone(Game& game);

    //========================================================//

    void tick() override;
};

//============================================================================//

}} // namespace sts::fighters
