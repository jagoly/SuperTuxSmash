#include <sqee/assert.hpp>
#include <sqee/misc/Algorithms.hpp>

#include "game/Actions.hpp"
#include "game/Fighter.hpp"
#include "game/FightSystem.hpp"

namespace algo = sq::algo;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

FightSystem::FightSystem() = default;

//============================================================================//

void FightSystem::tick()
{
    constexpr auto check_collision = [](auto first, auto second) -> bool
    {
        const float dist = maths::distance_squared(first.origin, second.origin);
        const float radiusSum = first.radius + second.radius;

        return dist < radiusSum * radiusSum;
    };

    constexpr auto compute_collision_point = [](auto first, auto second) -> Vec3F
    {
        const Vec3F direction = maths::normalize(second.origin - first.origin);

        const Vec3F pointA = first.origin + direction * first.radius;
        const Vec3F pointB = second.origin + direction * second.origin;

        return (pointA + pointB) * 0.5f;
    };

    //--------------------------------------------------------//

    const auto check_hit_bit = [this](HitBlob* hit, HitBlob* hurt) -> bool
    {
        uint32_t bits = mHitBitsArray[hit->fighter][hurt->fighter];
        return (bits & (1u << hit->offensive.group)) != 0u;
    };

    const auto fill_hit_bit = [this](HitBlob* hit, HitBlob* hurt) -> void
    {
        uint32_t& bits = mHitBitsArray[hit->fighter][hurt->fighter];
        bits = (bits | (1u << hit->offensive.group));
    };

    //--------------------------------------------------------//

    for (HitBlob* hit : mOffensiveBlobs)
    {
        for (HitBlob* hurt : mDamageableBlobs)
        {
            // check if both blobs belong to the same fighter
            if (hit->fighter == hurt->fighter) continue;

            // check if the blobs are not intersecting
            if (!check_collision(hit->sphere, hurt->sphere)) continue;

            // check if the group has already the hit fighter
            if (check_hit_bit(hit, hurt)) continue;

            // add the collision to the appropriate vector
            mCollisions[hit->fighter][hurt->fighter].emplace_back(hit, hurt);
        }
    }

    //--------------------------------------------------------//

    // the rest of this very bad, but somehow it works
    // todo: make less suck

    struct FinalResult
    {
        HitBlob* blob;
        Fighter& other;
        Vec3F point;
    };

    std::vector<FinalResult> finalResultVec;

    std::array<HitBlob*, 32> bestHitBlobs {};
    std::array<HitBlob*, 32> bestHurtBlobs {};

    for (auto& collisionsArr : mCollisions)
    {
        for (auto& collisionsVec : collisionsArr)
        {
            for (auto [hit, hurt] : collisionsVec)
            {
                const uint8_t group = hit->offensive.group;

                if ( bestHitBlobs[group] == nullptr ||
                     hit->offensive.damage > bestHitBlobs[group]->offensive.damage )
                {
                    bestHitBlobs[group] = hit;
                    bestHurtBlobs[group] = hurt;
                }
            }

            for (uint i = 0u; i < 32u; ++i)
            {
                if (HitBlob* hitBlob = bestHitBlobs[i])
                {
                    Fighter& other = *mFighters[bestHurtBlobs[i]->fighter];
                    Vec3F point = compute_collision_point(hitBlob->sphere, bestHurtBlobs[i]->sphere);

                    finalResultVec.push_back({ hitBlob, other, point });

                    fill_hit_bit(bestHitBlobs[i], bestHurtBlobs[i]);
                }
            }

            bestHitBlobs.fill(nullptr);
            bestHurtBlobs.fill(nullptr);

            collisionsVec.clear();
        }
    }

    for (auto& [blob, other, point] : finalResultVec)
    {
        blob->action->on_collide(blob, other, point);
    }
}

//============================================================================//

void FightSystem::add_fighter(Fighter& fighter)
{
    mFighters[fighter.index] = &fighter;
}

//============================================================================//

HitBlob* FightSystem::create_offensive_hit_blob(Fighter& fighter, Action& action, uint8_t group)
{
    auto blob = new (mBlobAllocator.allocate()) HitBlob(HitBlob::Type::Offensive, fighter.index, &action);

    blob->offensive.group = group;

    return mOffensiveBlobs.emplace_back(blob);
}

HitBlob* FightSystem::create_damageable_hit_blob(Fighter& fighter)
{
    auto blob = new (mBlobAllocator.allocate()) HitBlob(HitBlob::Type::Damageable, fighter.index, nullptr);

    return mDamageableBlobs.emplace_back(blob);
}

//============================================================================//

void FightSystem::delete_hit_blob(HitBlob* blob)
{
    if (blob->type == HitBlob::Type::Offensive)
        mOffensiveBlobs.erase(algo::find(mOffensiveBlobs, blob));

    if (blob->type == HitBlob::Type::Damageable)
        mDamageableBlobs.erase(algo::find(mDamageableBlobs, blob));

    mBlobAllocator.deallocate(blob);
}

//============================================================================//

void FightSystem::reset_offensive_blob_group(Fighter& fighter, uint8_t group)
{
    mHitBitsArray[fighter.index][0] &= ~uint32_t(1u << group);
    mHitBitsArray[fighter.index][1] &= ~uint32_t(1u << group);
    mHitBitsArray[fighter.index][2] &= ~uint32_t(1u << group);
    mHitBitsArray[fighter.index][3] &= ~uint32_t(1u << group);
}

//============================================================================//

FightSystem::BlobAllocator::BlobAllocator(uint maxBlobs)
{
    storage.reset(new FreeSlot[maxBlobs]);
    nextFreeSlot = storage.get();

    FreeSlot* iter = nextFreeSlot;
    for (uint i = 1u; i < maxBlobs; ++i)
    {
        iter->nextFreeSlot = std::next(iter);
        iter = iter->nextFreeSlot;
    }

    iter->nextFreeSlot = nullptr;
    finalSlot = iter;
}

HitBlob* FightSystem::BlobAllocator::allocate()
{
    SQASSERT(nextFreeSlot != nullptr, "too many blobs");

    auto ptr = reinterpret_cast<HitBlob*>(nextFreeSlot);
    nextFreeSlot = nextFreeSlot->nextFreeSlot;
    return ptr;
}

void FightSystem::BlobAllocator::deallocate(HitBlob* ptr)
{
    auto slot = reinterpret_cast<FreeSlot*>(ptr);

    SQASSERT ( slot >= storage.get() && slot <= finalSlot,
               "pointer outside of allocation range" );

    slot->nextFreeSlot = nextFreeSlot;
    nextFreeSlot = slot;
}
