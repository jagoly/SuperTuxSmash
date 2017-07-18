#include <sqee/assert.hpp>
#include <sqee/misc/Algorithms.hpp>

#include "game/Actions.hpp"
#include "game/FightSystem.hpp"

namespace algo = sq::algo;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

FightSystem::FightSystem() = default;

//============================================================================//

void FightSystem::tick()
{
    constexpr auto check_collision = [](auto first, auto second)
    {
        const float dist = maths::distance_squared(first.origin, second.origin);
        const float radiusSum = first.radius + second.radius;

        return dist < radiusSum * radiusSum;
    };

    //--------------------------------------------------------//

    for (auto offBlob : mOffensiveBlobs)
    {
        for (auto damBlob : mDamageableBlobs)
        {
            if (&offBlob->fighter == &damBlob->fighter) continue;

            if (check_collision(offBlob->sphere, damBlob->sphere) == true)
            {
                if (offBlob->action != nullptr)
                    offBlob->action->on_collide(offBlob, damBlob);
            }
        }
    }
}

//============================================================================//

HitBlob* FightSystem::create_hit_blob(HitBlob::Type type, Fighter& fighter, Action& action)
{
    auto blob = new (mBlobAllocator.allocate()) HitBlob(type, &fighter, &action);

    if (type == HitBlob::Type::Offensive)  mOffensiveBlobs.push_back(blob);
    if (type == HitBlob::Type::Damageable) mDamageableBlobs.push_back(blob);

    return blob;
}

HitBlob* FightSystem::create_hit_blob(HitBlob::Type type, Fighter& fighter)
{
    auto blob = new (mBlobAllocator.allocate()) HitBlob(type, &fighter, nullptr);

    if (type == HitBlob::Type::Offensive)  mOffensiveBlobs.push_back(blob);
    if (type == HitBlob::Type::Damageable) mDamageableBlobs.push_back(blob);

    return blob;
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
    SQASSERT(nextFreeSlot != nullptr, "too many hit blobs");

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
