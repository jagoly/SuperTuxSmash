#include "game/Entity.hpp"

using namespace sts;

//============================================================================//

Entity::Entity(const string& name) : name(name) {}

//============================================================================//

void Entity::base_tick_entity()
{
    mPreviousPosition = mCurrentPosition;
}
