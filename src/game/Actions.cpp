#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Algorithms.hpp>

#include "game/Actions.hpp"

using namespace sts;

//============================================================================//

Action::~Action() = default;

//============================================================================//

void Action::impl_start()
{
    on_start();

    mMethodIter = mMethodVec.begin();
    mCurrentFrame = 0u;
}

//============================================================================//

void Action::impl_cancel()
{
    on_cancel();
}

//============================================================================//

bool Action::impl_tick()
{
    if (on_tick()) { on_finish(); return true; }

    if (mMethodIter != mMethodVec.end())
    {
        if (mMethodIter->frame == mCurrentFrame)
        {
            mMethodIter->func();
            ++mMethodIter;
        }
    }

    ++mCurrentFrame; return false;
}

//============================================================================//

std::function<void()>& Action::add_frame_method(uint frame)
{
    SQASSERT(mMethodVec.empty() || frame > mMethodVec.back().frame, "");

    return mMethodVec.emplace_back(frame).func;
}

//============================================================================//

void Action::jump_to_frame(uint frame)
{
    auto predicate = [&](auto& method) { return method.frame >= frame; };
    mMethodIter = sq::algo::find_if(mMethodVec, predicate);

    mCurrentFrame = frame;
}

//============================================================================//

void Actions::tick_active_action()
{
    if (mActiveAction && mActiveAction->impl_tick())
    {
        mActiveType = Action::Type::None;
        mActiveAction = nullptr;
    }
}

//============================================================================//

void DumbAction::on_start() { sq::log_info(" start: %s", mMessage); }

void DumbAction::on_finish() { sq::log_info("finish: %s", mMessage); }

void DumbAction::on_cancel() { sq::log_info("cancel: %s", mMessage); }

bool DumbAction::on_tick() { return get_current_frame() == 12u; }
