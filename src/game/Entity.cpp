#include "Entity.hpp"

using namespace sts;

//============================================================================//

Entity::Entity(string name, Game& game) : name(name), game(game) {}

//============================================================================//

void Entity::base_tick_Entity()
{
    mPreviousPosition = mCurrentPosition;
}
