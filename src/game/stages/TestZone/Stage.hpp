#pragma once

#include <game/Stage.hpp>

namespace sts { namespace stages {

//============================================================================//

class TestZone final : public sts::Stage
{
public:

    //========================================================//

    void tick() override;
};

//============================================================================//

}} // namespace sts::fighters
