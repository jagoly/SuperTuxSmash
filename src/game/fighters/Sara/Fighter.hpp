#pragma once

#include <game/Fighter.hpp>

namespace sts { namespace fighters {

//============================================================================//

class Sara_Fighter final : public sts::Fighter
{
public:

    //========================================================//

    Sara_Fighter(Stage& stage);

    ~Sara_Fighter() override;

    //========================================================//

    void tick() override;

    //========================================================//

    friend struct Sara_Attacks;
};

//============================================================================//

}} // namespace sts::fighters
