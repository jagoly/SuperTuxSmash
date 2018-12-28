#include "game/ActionFuncs.hpp"

#include "game/Actions.hpp"
#include "game/Fighter.hpp"

#include <sqee/debug/Logging.hpp>

using namespace sts;

using sq::literals::operator""_fmt_;

//============================================================================//

#define VERIFY(condition, fmtStr, ...) \
do { \
    if (!(condition)) \
    { \
        return fmtStr##_fmt_(__VA_ARGS__); \
    } \
} while (false)

//============================================================================//

void ActionFuncs::enable_blob(Action& action, PoolKey key)
{
    action.world.enable_hit_blob(action.blobs[key]);
};


String ActionFuncsValidate::enable_blob(Action& action, PoolKey key)
{
    VERIFY(action.blobs.contains(key), "HitBlob '%s' does not exist", key);
    return String();
}

//----------------------------------------------------------------------------//

void ActionFuncs::disable_blob(Action& action, PoolKey key)
{
    action.world.disable_hit_blob(action.blobs[key]);
};

String ActionFuncsValidate::disable_blob(Action& action, PoolKey key)
{
    VERIFY(action.blobs.contains(key), "HitBlob '%s' does not exist", key);
    return String();
}

//----------------------------------------------------------------------------//

void ActionFuncs::add_velocity(Action& action, float fwd, float up)
{
    Fighter& fighter = action.fighter;
    fighter.mVelocity.x += float(fighter.current.facing) * fwd;
    fighter.mVelocity.y += up;
}

String ActionFuncsValidate::add_velocity(Action& /*action*/, float /*fwd*/, float /*up*/)
{
    return String();
}

//----------------------------------------------------------------------------//

void ActionFuncs::finish_action(Action& action)
{
    action.finished = true;
};

String ActionFuncsValidate::finish_action(Action& /*action*/)
{
    return String();
}

//----------------------------------------------------------------------------//

void ActionFuncs::emit_particles(Action& action, PoolKey key, uint count)
{
    action.emitters[key]->generate(action.world.get_particle_system(), count);
}

String ActionFuncsValidate::emit_particles(Action& action, PoolKey key, uint /*count*/)
{
    VERIFY(action.emitters.contains(key), "Emitter '%s' does not exist", key);
    return String();
}
