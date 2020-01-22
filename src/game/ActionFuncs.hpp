#pragma once

#include <sqee/misc/Builtins.hpp>
#include <sqee/misc/TinyString.hpp>

namespace sts {

//====== Forward Declarations and Aliases ====================================//

class Action;

using PoolKey = TinyString;

//============================================================================//

// These functions are called in game and may crash if paramaters are invalid.
struct ActionFuncs final
{
    static void enable_blob(Action& action, PoolKey key);
    static void disable_blob(Action& action, PoolKey key);

    static void add_velocity(Action& action, float fwd, float up);

    static void set_position(Action& action, float x, float y);

    static void finish_action(Action& action);

    static void emit_particles(Action& action, PoolKey key, uint count);
};

// These functions are called when building procedures to ensure that paramaters are valid.
struct ActionFuncsValidate final
{
    static String enable_blob(Action& action, PoolKey key);
    static String disable_blob(Action& action, PoolKey key);

    static String add_velocity(Action& action, float fwd, float up);

    static String set_position(Action& action, float x, float y);

    static String finish_action(Action& action);

    static String emit_particles(Action& action, PoolKey key, uint count);
};

//============================================================================//

} // namespace sts
