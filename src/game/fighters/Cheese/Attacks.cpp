#include <game/fighters/Cheese/Attacks.hpp>

using namespace sts::fighters;

//============================================================================//

Cheese_Attacks::Cheese_Attacks(Cheese_Fighter& fighter) : Attacks("Cheese", fighter)
{
    this->make_default_attacks();
    this->load_from_json();
}
