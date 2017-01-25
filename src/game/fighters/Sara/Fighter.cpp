#include <game/fighters/Sara/Attacks.hpp>
#include <game/fighters/Sara/Fighter.hpp>

using namespace sts::fighters;

//============================================================================//

Sara_Fighter::Sara_Fighter(Stage& stage) : Fighter("Sara", stage)
{
    attacks = std::make_unique<Sara_Attacks>(*this);
}

Sara_Fighter::~Sara_Fighter()
{

}

//============================================================================//

void Sara_Fighter::tick()
{
    this->impl_tick_base();
}
