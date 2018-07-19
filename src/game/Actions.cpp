#include <sqee/helpers.hpp>
#include <sqee/misc/Parsing.hpp>

#include <sqee/debug/Logging.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

#include "game/Fighter.hpp"
#include "game/ActionBuilder.hpp"
#include "game/ActionFuncs.hpp"

#include "game/Actions.hpp"

namespace maths = sq::maths;

using json = nlohmann::json;
using sq::literals::operator""_fmt_;
using namespace sts;

//============================================================================//

Action::Action(FightWorld& world, Fighter& fighter, Type type)
    : world(world), fighter(fighter), type(type)
    , path("assets/fighters/%s/actions/%s.json"_fmt_(fighter.get_name(), enum_to_string(type)))
    , blobs(world.get_hit_blob_allocator()), emitters(world.get_emitter_allocator())
{
    ActionBuilder::load_from_json(*this);
}

Action::~Action() = default;

//----------------------------------------------------------------------------//

void Action::do_start()
{
    mTimelineIter = timeline.begin();
    mCurrentFrame = 0u;
    finished = false;
}

bool Action::do_tick()
{
    if (mTimelineIter != timeline.end() && mTimelineIter->frame == mCurrentFrame)
    {
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

        for (auto& cmd : mTimelineIter->commands)
            cmd.func(*this);

        mTimelineIter = std::next(mTimelineIter);
    }

    mCurrentFrame += 1u;

    return finished;
}
