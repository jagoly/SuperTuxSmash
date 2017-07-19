#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Algorithms.hpp>
#include <sqee/assert.hpp>

#include "game/Fighter.hpp"
#include "game/FightSystem.hpp"
#include "game/Actions.hpp"

using namespace sts;

//============================================================================//

Action::~Action()
{
    SQASSERT ( sq::algo::exists_not(blobs, nullptr) == false,
               "forgot to delete one or more hit blobs" );
}

//============================================================================//

bool Action::impl_do_tick()
{
    if (on_tick(mCurrentFrame)) { on_finish(); return true; }
    else { ++mCurrentFrame; return false; }
}

//============================================================================//

void Action::jump_to_frame(uint frame)
{
    mCurrentFrame = frame;
}

//============================================================================//

void Actions::tick_active_action()
{
    if (mActiveAction && mActiveAction->impl_do_tick())
    {
        mActiveType = Action::Type::None;
        mActiveAction = nullptr;
    }
}

//============================================================================//

void DumbAction::on_start() { sq::log_info(" start: " + message); }

bool DumbAction::on_tick(uint frame) { return frame == 12u; }

void DumbAction::on_collide(HitBlob*, Fighter&, Vec3F) {}

void DumbAction::on_finish() { sq::log_info("finish: " + message); }
