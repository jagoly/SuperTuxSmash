#pragma once

#include <game/Fighter.hpp>

namespace sts { namespace fighters {

//============================================================================//

class Cheese_Fighter final : public sts::Fighter
{
public:

    //========================================================//

    Cheese_Fighter(Stage& stage);

    ~Cheese_Fighter() override;

    //========================================================//

    void tick() override;

    //========================================================//

    friend struct Cheese_Attacks;
};

//============================================================================//

}} // namespace sts::fighters
