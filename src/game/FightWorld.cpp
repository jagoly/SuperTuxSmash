#include "game/FightWorld.hpp"

#include "game/Actions.hpp"
#include "game/Fighter.hpp"
#include "game/Stage.hpp"
#include "game/private/PrivateWorld.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Culling.hpp>
#include <sqee/misc/Algorithms.hpp>

namespace algo = sq::algo;
namespace maths = sq::maths;

using namespace sts;

//============================================================================//

FightWorld::FightWorld(const Globals& globals)
    : globals(globals)
    , mHurtBlobAlloc(globals.editorMode ? 128 * 64 : 128)
    , mHitBlobAlloc(globals.editorMode ? 1024 * 64 : 1024)
    , mEmitterAlloc(globals.editorMode ? 1024 * 64 : 1024)
{
    mMessageBus.add_message_source<message::fighter_action_finished>();

    impl = std::make_unique<PrivateWorld>(*this);
}

FightWorld::~FightWorld() = default;

//============================================================================//

void FightWorld::tick()
{
    mStage->tick();

    for (auto& fighter : mFighters)
    {
        if (fighter != nullptr)
            fighter->tick();
    }

    impl->tick();

    for (auto& fighter : mFighters)
    {
        if (fighter != nullptr)
            mStage->check_boundary(*fighter);
    }

    mParticleSystem.update_and_clean();
}

//============================================================================//

void FightWorld::set_stage(UniquePtr<Stage> stage)
{
    mStage = std::move(stage);
}

void FightWorld::add_fighter(UniquePtr<Fighter> fighter)
{
    mFighters[fighter->index] = std::move(fighter);
}

//============================================================================//

const Vector<HitBlob*>& FightWorld::get_hit_blobs() const
{
    return impl->enabledHitBlobs;
}

const Vector<HurtBlob*>& FightWorld::get_hurt_blobs() const
{
    return impl->enabledHurtBlobs;
}

//============================================================================//

// for now these can silently fail, so as to not crash in the action editor
// possibly these could throw exceptions which the editor can catch and display

void FightWorld::enable_hit_blob(HitBlob* blob)
{
    const auto iter = algo::find(impl->enabledHitBlobs, blob);
    if (iter != impl->enabledHitBlobs.end()) return;

    impl->enabledHitBlobs.push_back(blob);
}

void FightWorld::disable_hit_blob(HitBlob* blob)
{
    const auto iter = algo::find(impl->enabledHitBlobs, blob);
    if (iter == impl->enabledHitBlobs.end()) return;

    impl->enabledHitBlobs.erase(iter);
}

void FightWorld::enable_hurt_blob(HurtBlob *blob)
{
    const auto iter = algo::find(impl->enabledHurtBlobs, blob);
    if (iter != impl->enabledHurtBlobs.end()) return;

    impl->enabledHurtBlobs.push_back(blob);
}

void FightWorld::disable_hurt_blob(HurtBlob* blob)
{
    const auto iter = algo::find(impl->enabledHurtBlobs, blob);
    if (iter == impl->enabledHurtBlobs.end()) return;

    impl->enabledHurtBlobs.erase(iter);
}

//============================================================================//

void FightWorld::reset_all_hit_blob_groups(Fighter& fighter)
{
    impl->hitBitsArray[fighter.index].fill(uint32_t(0u));
}

void FightWorld::disable_all_hit_blobs(Fighter& fighter)
{
    const auto predicate = [&](HitBlob* blob) { return blob->fighter == &fighter; };
    const auto end = std::remove_if(impl->enabledHitBlobs.begin(), impl->enabledHitBlobs.end(), predicate);
    impl->enabledHitBlobs.erase(end, impl->enabledHitBlobs.end());
}

void FightWorld::disable_all_hurt_blobs(Fighter &fighter)
{
    const auto predicate = [&](HurtBlob* blob) { return blob->fighter == &fighter; };
    const auto end = std::remove_if(impl->enabledHurtBlobs.begin(), impl->enabledHurtBlobs.end(), predicate);
    impl->enabledHurtBlobs.erase(end, impl->enabledHurtBlobs.end());
}

//============================================================================//

SceneData FightWorld::compute_scene_data() const
{
    SceneData result;

    result.view = { Vec2F(+INFINITY), Vec2F(-INFINITY) };

    for (const auto& fighter : mFighters)
    {
        if (fighter == nullptr) continue;

        const Vec2F origin = fighter->get_diamond().centre();

        result.view.min = maths::min(result.view.min, origin);
        result.view.max = maths::max(result.view.max, origin);
    }

    result.inner = { Vec2F(-14.f,  -6.f), Vec2F(+14.f, +14.f) };
    result.outer = { Vec2F(-18.f, -10.f), Vec2F(+18.f, +18.f) };

    result.view.min = maths::max(result.view.min, result.inner.min);
    result.view.max = maths::min(result.view.max, result.inner.max);

    return result;
}

//============================================================================//

void FightWorld::editor_disable_all_hurtblobs()
{
    impl->enabledHurtBlobs.clear();
}
