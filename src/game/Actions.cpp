#include <sqee/debug/Logging.hpp>

#include "Actions.hpp"

using namespace sts;

//============================================================================//

Action::Action(Action::Type type) : type(type) {}

Action::~Action() = default;

//============================================================================//

void Action::on_start()
{
    sq::log_info(" start: %s", props.message);
    mTimeLeft = props.time;
}

bool Action::on_tick()
{
    //sq::log_info("  tick: %s", props.message);
    return --mTimeLeft == 0u;
}

void Action::on_finish()
{
    sq::log_info("finish: %s", props.message);
}
