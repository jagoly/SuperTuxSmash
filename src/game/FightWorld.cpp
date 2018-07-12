#include <sqee/assert.hpp>
#include <sqee/misc/Algorithms.hpp>
#include <sqee/maths/Culling.hpp>

#include "game/Stage.hpp"
#include "game/Actions.hpp"
#include "game/Fighter.hpp"

#include "game/private/PrivateWorld.hpp"

#include "game/FightWorld.hpp"

namespace algo = sq::algo;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

FightWorld::FightWorld()
{
    impl = std::make_unique<PrivateWorld>(*this);
}

FightWorld::~FightWorld() = default;

//============================================================================//

void FightWorld::tick()
{
    mStage->tick();

    for (auto& fighter : mFighters)
        if (fighter != nullptr)
            fighter->tick();

    //--------------------------------------------------------//

    impl->tick();

    //--------------------------------------------------------//

    for (auto& fighter : mFighters)
    {
        if (fighter == nullptr) continue;
        mStage->check_boundary(*fighter);
    }

    mParticleSet.update_and_clean();
}

//============================================================================//

void FightWorld::set_stage(unique_ptr<Stage> stage)
{
    mStage = std::move(stage);
}

void FightWorld::add_fighter(unique_ptr<Fighter> fighter)
{
    mFighters[fighter->index] = std::move(fighter);
}

//============================================================================//

sq::PoolAllocator<HitBlob>& FightWorld::get_hit_blob_allocator() { return impl->hitBlobAlloc; }

sq::PoolAllocator<HurtBlob>& FightWorld::get_hurt_blob_allocator() { return impl->hurtBlobAlloc; }

const std::vector<HitBlob*>& FightWorld::get_hit_blobs() const { return impl->enabledHitBlobs; }

const std::vector<HurtBlob*>& FightWorld::get_hurt_blobs() const { return impl->enabledHurtBlobs; }

//============================================================================//

HurtBlob* FightWorld::create_hurt_blob(Fighter& fighter)
{
    HurtBlob* blob = impl->hurtBlobAlloc.allocate(fighter);
    return impl->enabledHurtBlobs.emplace_back(blob);
}

void FightWorld::delete_hurt_blob(HurtBlob* blob)
{
    impl->enabledHurtBlobs.erase(algo::find(impl->enabledHurtBlobs, blob));
    impl->hurtBlobAlloc.deallocate(blob);
}

//============================================================================//

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
