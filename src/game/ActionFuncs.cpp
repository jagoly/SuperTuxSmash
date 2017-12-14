#include "game/Actions.hpp"
#include "game/Fighter.hpp"

#include "game/ActionFuncs.hpp"

using namespace sts;

//============================================================================//

void ActionFuncs::enable_blob(Action& action, PoolKey key)
{
    action.world.enable_hit_blob(action.blobs[key]);
};

void ActionFuncs::disable_blob(Action& action, PoolKey key)
{
    action.world.disable_hit_blob(action.blobs[key]);
};

//----------------------------------------------------------------------------//

void ActionFuncs::add_velocity(Action& action, float fwd, float up)
{
    Fighter& fighter = action.fighter;
    fighter.mVelocity.x += float(fighter.current.facing) * fwd;
    fighter.mVelocity.y += up;
}

//----------------------------------------------------------------------------//

void ActionFuncs::finish_action(Action& action)
{
    action.world.reset_all_hit_blob_groups(action.fighter);
    action.finished = true;
};

//----------------------------------------------------------------------------//
