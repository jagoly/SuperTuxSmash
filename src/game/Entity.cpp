#include "game/Entity.hpp"

using namespace sts;

//============================================================================//

Entity::Entity(FightSystem& system) : mFightSystem(system) {}

//============================================================================//

void Entity::base_tick_entity()
{
    mPreviousPosition = mCurrentPosition;
}
