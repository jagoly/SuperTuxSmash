#pragma once

#include <sqee/misc/PoolTools.hpp>

namespace sts {

//====== Forward Declarations and Aliases ====================================//

using PoolKey = sq::TinyString<15>;

class Action;

//============================================================================//

struct ActionFuncs final
{
    static void enable_blob(Action& action, PoolKey key);
    static void disable_blob(Action& action, PoolKey key);

    static void add_velocity(Action& action, float fwd, float up);

    static void finish_action(Action& action);
};

//============================================================================//

} // namespace sts
