#include <sqee/debug/Logging.hpp>

#include <game/Fighter.hpp>

#include "Actions.hpp"

using namespace sts;

//============================================================================//

namespace { // anonymous

bool impl_debug_action(Fighter& fighter, uint& timeLeft, string action)
{
    if (timeLeft == 0u)
    {
        sq::log_info(" start: %s %s", fighter.name, action);
        timeLeft = 6u;
    }

    else if (--timeLeft == 0u)
    {
        sq::log_info("finish: %s %s", fighter.name, action);
        return true;
    }

    return false;
}

} // anonymous namespace

//============================================================================//

DebugActions::DebugActions(Fighter& fighter) : mFighter(fighter) {}

//============================================================================//

bool DebugActions::fn_neutral_first() { return impl_debug_action(mFighter, mTimeLeft, "neutral_first"); }

bool DebugActions::fn_tilt_down() { return impl_debug_action(mFighter, mTimeLeft, "tilt_down"); }
bool DebugActions::fn_tilt_forward() { return impl_debug_action(mFighter, mTimeLeft, "tilt_forward"); }
bool DebugActions::fn_tilt_up() { return impl_debug_action(mFighter, mTimeLeft, "tilt_up"); }

bool DebugActions::fn_air_back() { return impl_debug_action(mFighter, mTimeLeft, "air_back"); }
bool DebugActions::fn_air_down() { return impl_debug_action(mFighter, mTimeLeft, "air_down"); }
bool DebugActions::fn_air_forward() { return impl_debug_action(mFighter, mTimeLeft, "air_forward"); }
bool DebugActions::fn_air_neutral() { return impl_debug_action(mFighter, mTimeLeft, "air_neutral"); }
bool DebugActions::fn_air_up() { return impl_debug_action(mFighter, mTimeLeft, "air_up"); }

bool DebugActions::fn_dash_attack() { return impl_debug_action(mFighter, mTimeLeft, "dash_attack"); }
