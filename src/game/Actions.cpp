#include <sqee/debug/Logging.hpp>

#include "game/Actions.hpp"

using namespace sts;

//============================================================================//

Action::~Action() = default;

//============================================================================//

void Action::start()
{
    this->on_start();

    mMethodIter = mMethodVec.begin();
    mCurrentFrame = 0u;
}

//============================================================================//

void Action::cancel()
{
    this->on_cancel();
}

//============================================================================//

bool Action::tick()
{
    if (this->on_tick())
    {
        this->on_finish();
        return true;
    }

    if (mMethodIter != mMethodVec.end())
    {
        if (mMethodIter->frame == mCurrentFrame)
        {
            mMethodIter->func();
            ++mMethodIter;
        }
    }

    ++mCurrentFrame;
    return false;
}

//============================================================================//

void DumbAction::on_start() { sq::log_info(" start: %s", mMessage); }

void DumbAction::on_finish() { sq::log_info("finish: %s", mMessage); }

void DumbAction::on_cancel() { sq::log_info("cancel: %s", mMessage); }

bool DumbAction::on_tick() { return get_current_frame() == 12u; }
