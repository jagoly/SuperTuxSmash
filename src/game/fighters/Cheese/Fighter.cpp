#include <game/fighters/Cheese/Attacks.hpp>
#include <game/fighters/Cheese/Fighter.hpp>

using namespace sts::fighters;

//============================================================================//

Cheese_Fighter::Cheese_Fighter(Stage& stage) : Fighter("Cheese", stage)
{
    attacks = std::make_unique<Cheese_Attacks>(*this);
}

Cheese_Fighter::~Cheese_Fighter()
{

}

//============================================================================//

void Cheese_Fighter::tick()
{
    this->impl_tick_base();
}
