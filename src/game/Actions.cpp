#include <sqee/helpers.hpp>
#include <sqee/misc/Parsing.hpp>

#include <sqee/debug/Logging.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

#include "game/Fighter.hpp"
#include "game/ActionBuilder.hpp"
#include "game/ActionFuncs.hpp"

#include "game/Actions.hpp"

namespace algo = sq::algo;
namespace maths = sq::maths;
using sq::build_string;

using namespace sts;

//============================================================================//

Action::Action(FightWorld& world, Fighter& fighter, Type type)
    : world(world), fighter(fighter), type(type)
    , path(build_string("assets/fighters/", fighter.get_name(), "/actions/", enum_to_string(type), ".json"))
    , blobs(world.get_hit_blob_allocator()), emitters(world.get_emitter_allocator())
{
    ActionBuilder::load_from_json(*this);
}

Action::~Action() = default;

//----------------------------------------------------------------------------//

void Action::do_start()
{
    mCommandIter = commands.begin();
    mCurrentFrame = 0u;
    finished = false;
}

bool Action::do_tick()
{
    SQASSERT(mCommandIter != commands.end(), "finish didn't get called!");

    // todo: this doesn't need to happen every frame
    for (auto& [key, emitter] : emitters)
    {
        Mat4F matrix = fighter.get_model_matrix();

        if (emitter->bone >= 0)
        {
            const auto& matrices = fighter.get_bone_matrices();
            const auto& boneMatrix = matrices[uint(emitter->bone)];
            matrix *= maths::transpose(Mat4F(boneMatrix));
        }

        emitter->emitPosition = Vec3F(matrix * Vec4F(emitter->origin, 1.f));
        emitter->emitVelocity = Vec3F(fighter.get_velocity().x * 0.2f, 0.f, 0.f);
    }

    for (; mCommandIter != commands.end(); ++mCommandIter)
    {
        if (mCommandIter->frames.empty()) continue;
        if (mCurrentFrame <= mCommandIter->frames.back()) break;
    }

    for (auto iter = mCommandIter; iter != commands.end(); ++iter)
    {
        if (iter->frames.empty()) continue;
        if (mCurrentFrame < iter->frames.front()) break;

        if (algo::exists(iter->frames, mCurrentFrame))
            iter->func(*this);
    }

    mCurrentFrame += 1u;

    return finished;
}

//----------------------------------------------------------------------------//

void Action::do_finish()
{
    world.disable_all_hit_blobs(fighter);
    world.reset_all_hit_blob_groups(fighter);
}
