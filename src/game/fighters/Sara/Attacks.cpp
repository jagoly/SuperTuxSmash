#include <game/fighters/Sara/Attacks.hpp>

using namespace sts::fighters;

//============================================================================//

Sara_Attacks::Sara_Attacks(Sara_Fighter& fighter) : Attacks("Sara", fighter)
{
    this->make_default_attacks();
    this->load_from_json();
}
