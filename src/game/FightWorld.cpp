#include <sqee/assert.hpp>
#include <sqee/misc/Algorithms.hpp>
#include <sqee/maths/Culling.hpp>

#include "game/Stage.hpp"
#include "game/Actions.hpp"
#include "game/Fighter.hpp"

#include "game/FightWorld.hpp"

namespace algo = sq::algo;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

FightWorld::FightWorld() = default;

//============================================================================//

void FightWorld::tick()
{
    mStage->tick();

    for (auto& fighter : mFighters)
        if (fighter != nullptr)
            fighter->tick();


    //== misc helper lambda functions ==================================//

    const auto check_hit_bit = [this](HitBlob* hit, HurtBlob* hurt) -> bool
    {
        uint32_t bits = mHitBitsArray[hit->fighter->index][hurt->fighter->index];
        return (bits & (1u << hit->group)) != 0u;
    };

    const auto fill_hit_bit = [this](HitBlob* hit, HurtBlob* hurt) -> void
    {
        uint32_t& bits = mHitBitsArray[hit->fighter->index][hurt->fighter->index];
        bits = (bits | (1u << hit->group));
    };


    //== update world-space shapes of hit blobs & hurt blobs ===========//

    for (HitBlob* blob : mEnabledHitBlobs)
    {
        Mat4F matrix = blob->fighter->get_model_matrix();

        if (blob->bone >= 0)
        {
            const auto& matrices = blob->fighter->get_bone_matrices();
            const auto& boneMatrix = matrices[uint(blob->bone)];
            matrix *= maths::transpose(Mat4F(boneMatrix));
        }

        blob->sphere.origin = Vec3F(matrix * Vec4F(blob->origin, 1.f));
        blob->sphere.radius = blob->radius;// * matrix[0][0];
    }

    for (HurtBlob* blob : mHurtBlobs)
    {
        Mat4F matrix = blob->fighter->get_model_matrix();

        if (blob->bone >= 0)
        {
            const auto& matrices = blob->fighter->get_bone_matrices();
            const auto& boneMatrix = matrices[uint(blob->bone)];
            matrix *= maths::transpose(Mat4F(boneMatrix));
        }

        blob->capsule.originA = Vec3F(matrix * Vec4F(blob->originA, 1.f));
        blob->capsule.originB = Vec3F(matrix * Vec4F(blob->originB, 1.f));
        blob->capsule.radius = blob->radius;// * matrix[0][0];
    }


    //== find all collisions between hit and hurt blobs ================//

    for (HitBlob* hit : mEnabledHitBlobs)
    {
        for (HurtBlob* hurt : mHurtBlobs)
        {
            // check if both blobs belong to the same fighter
            if (hit->fighter == hurt->fighter) continue;

            // check if the blobs are not intersecting
            if (maths::intersect_sphere_capsule(hit->sphere, hurt->capsule) == -1) continue;

            // check if the group has already hit the fighter
            if (check_hit_bit(hit, hurt)) continue;

            // add the collision to the appropriate vector
            mCollisions[hit->fighter->index][hurt->fighter->index].push_back({hit, hurt});
        }
    }

    //--------------------------------------------------------//

    // the rest of this very bad, but somehow it works
    // todo: make less suck

    struct FinalResult
    {
        HitBlob* blob;
        Fighter& other;
    };

    std::vector<FinalResult> finalResultVec;

    std::array<HitBlob*, 32> bestHitBlobs {};
    std::array<HurtBlob*, 32> bestHurtBlobs {};

    for (auto& collisionsArr : mCollisions)
    {
        for (auto& collisionsVec : collisionsArr)
        {
            for (auto [hit, hurt] : collisionsVec)
            {
                const uint8_t group = hit->group;

                if ( bestHitBlobs[group] == nullptr ||
                     hit->damage > bestHitBlobs[group]->damage )
                {
                    bestHitBlobs[group] = hit;
                    bestHurtBlobs[group] = hurt;
                }
            }

            for (uint i = 0u; i < 32u; ++i)
            {
                if (HitBlob* hitBlob = bestHitBlobs[i])
                {
                    finalResultVec.push_back({ hitBlob, *bestHurtBlobs[i]->fighter });

                    fill_hit_bit(bestHitBlobs[i], bestHurtBlobs[i]);
                }
            }

            bestHitBlobs.fill(nullptr);
            bestHurtBlobs.fill(nullptr);

            collisionsVec.clear();
        }
    }

    for (auto& [blob, other] : finalResultVec)
    {
        blob->action->on_collide(blob, other);
    }
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

HurtBlob* FightWorld::create_hurt_blob(Fighter& fighter)
{
    HurtBlob* blob = mHurtBlobAlloc.allocate(fighter);

    return mHurtBlobs.emplace_back(blob);
}

void FightWorld::delete_hurt_blob(HurtBlob* blob)
{
    mHurtBlobs.erase(algo::find(mHurtBlobs, blob));

    mHurtBlobAlloc.deallocate(blob);
}

//============================================================================//

void FightWorld::enable_hit_blob(HitBlob* blob)
{
    const auto iter = algo::find(mEnabledHitBlobs, blob);
    if (iter != mEnabledHitBlobs.end()) return;

    mEnabledHitBlobs.push_back(blob);
}

void FightWorld::disable_hit_blob(HitBlob* blob)
{
    const auto iter = algo::find(mEnabledHitBlobs, blob);
    if (iter == mEnabledHitBlobs.end()) return;

    mEnabledHitBlobs.erase(iter);
}

//============================================================================//

void FightWorld::reset_all_hit_blob_groups(Fighter& fighter)
{
    mHitBitsArray[fighter.index].fill(uint32_t(0u));
}

void FightWorld::disable_all_hit_blobs(Fighter& fighter)
{
    for (auto it = mEnabledHitBlobs.rbegin(); it != mEnabledHitBlobs.rend(); ++it)
        if ((*it)->fighter == &fighter) mEnabledHitBlobs.erase(it.base());
}

//============================================================================//

sq::maths::Sphere FightWorld::compute_camera_view_bounds() const
{
    Vec2F maxCentre = Vec2F();
    float maxDistance = -INFINITY;

    for (const auto& fighterA : mFighters)
    {
        if (fighterA == nullptr) continue;

        const Vec2F originA = Vec2F(fighterA->get_model_matrix()[3]);

        for (const auto& fighterB : mFighters)
        {
            if (fighterB == nullptr) continue;
            if (fighterA == fighterB) continue;

            const Vec2F originB = Vec2F(fighterB->get_model_matrix()[3]);

            const float dist = maths::distance(originA, originB);

            if (dist > maxDistance)
            {
                maxCentre = (originA + originB) * 0.5f;
                maxDistance = dist;
            }
        }
    }

    return { Vec3F(maxCentre, 0.f), maxDistance * 0.5f };
}