#include "game/Actions.hpp"

#include "main/Globals.hpp"

#include "game/Fighter.hpp"
#include "game/ActionBuilder.hpp"
#include "game/ActionFuncs.hpp"

#include <sqee/helpers.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/misc/Parsing.hpp>

namespace maths = sq::maths;

using namespace sts;

//============================================================================//

Action::Action(FightWorld& world, Fighter& fighter, ActionType type, String path)
    : world(world), fighter(fighter), type(type), path(path)
    , blobs(world.get_hit_blob_allocator()), emitters(world.get_emitter_allocator()) {}

Action::~Action() = default;

//----------------------------------------------------------------------------//

void Action::do_start()
{
    mCurrentFrame = 0u;
    finished = false;
}

bool Action::do_tick()
{
    SQASSERT(world.globals.editorMode || mCurrentFrame < timeline.size(), "finish didn't get called!");

    // todo: this doesn't need to happen every frame
    for (auto& [key, emitter] : emitters)
    {
        Mat4F matrix = fighter.get_model_matrix();

        if (emitter.bone >= 0)
        {
            const auto& matrices = fighter.get_bone_matrices();
            const auto& boneMatrix = matrices[uint(emitter.bone)];
            matrix *= maths::transpose(Mat4F(boneMatrix));
        }

        emitter.emitPosition = Vec3F(matrix * Vec4F(emitter.origin, 1.f));
        emitter.emitVelocity = Vec3F(fighter.get_velocity().x * 0.2f, 0.f, 0.f);
    }

    for (const Procedure& procedure : timeline[mCurrentFrame].procedures)
    {
        for (const Command& command : procedure.commands)
            command(*this);
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

//----------------------------------------------------------------------------//

void Action::rebuild_timeline()
{
    timeline.clear();

    for (const auto& [name, procedure] : procedures)
    {
        for (const uint16_t frame : procedure.meta.frames)
        {
            if (timeline.size() < frame + 1u) timeline.resize(frame + 1u);
            timeline[frame].procedures.push_back(procedure);
        }
    }

    for (TimelineItem& item : timeline)
        item.procedures.shrink_to_fit();

    timeline.shrink_to_fit();
}

//============================================================================//

bool Action::has_changes(const Action& reference) const
{
    if (blobs != reference.blobs) return true;
    if (emitters != reference.emitters) return true;
    if (procedures != reference.procedures) return true;
    return false;
}

void Action::apply_changes(const Action &source)
{
    blobs.clear();
    emitters.clear();
    procedures.clear();

    for (const auto& [key, blob] : source.blobs)
        blobs.try_emplace(key, blob);

    for (const auto& [key, emitter] : source.emitters)
        emitters.try_emplace(key, emitter);

    for (const auto& [key, procedure] : source.procedures)
        procedures.try_emplace(key, procedure);

    rebuild_timeline();
}

UniquePtr<Action> Action::clone() const
{
    auto result = std::make_unique<Action>(world, fighter, type, path);

    result->apply_changes(*this);

    return result;
}
