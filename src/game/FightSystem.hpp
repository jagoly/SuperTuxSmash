#pragma once

#include <sqee/builtins.hpp>

#include "game/HitBlob.hpp"

//============================================================================//

namespace sts {

class FightSystem final : sq::NonCopyable
{
public: //====================================================//

    FightSystem();

    void tick();

    //--------------------------------------------------------//

    /// Add a fighter to the game.
    void add_fighter(Fighter& fighter);

    //--------------------------------------------------------//

    /// Create a new offensive hit blob in the specified group.
    HitBlob* create_offensive_hit_blob(Fighter& fighter, Action& action, uint8_t group);

    /// Create a new damageable hit blob.
    HitBlob* create_damageable_hit_blob(Fighter& fighter);

    /// Delete an existing hit blob.
    void delete_hit_blob(HitBlob* blob);

    /// Reset one of a fighter's offensive blob groups.
    void reset_offensive_blob_group(Fighter& fighter, uint8_t group);

    //--------------------------------------------------------//

    const auto& get_offensive_blobs() const { return mOffensiveBlobs; }
    const auto& get_damageable_blobs() const { return mDamageableBlobs; }

private: //===================================================//

    // todo: should this class instead own the fighters?
    std::array<Fighter*, 4> mFighters;

    //--------------------------------------------------------//

    std::vector<HitBlob*> mOffensiveBlobs;
    std::vector<HitBlob*> mDamageableBlobs;

    //--------------------------------------------------------//

    std::array<std::array<uint32_t, 4>, 4> mHitBitsArray;
    static_assert(sizeof(mHitBitsArray) == 64u);

    //--------------------------------------------------------//

    using Collision = std::pair<HitBlob*, HitBlob*>;

    std::array<std::array<std::vector<Collision>, 4u>, 4u> mCollisions;

    //--------------------------------------------------------//

    struct BlobAllocator
    {
        BlobAllocator(uint maxBlobs);

        HitBlob* allocate();

        void deallocate(HitBlob* ptr);

        struct alignas(64) FreeSlot
        {
            FreeSlot* nextFreeSlot;
            char _padding[56];
        };

        unique_ptr<FreeSlot[]> storage;

        FreeSlot* nextFreeSlot;
        FreeSlot* finalSlot;
    };

    //--------------------------------------------------------//

    BlobAllocator mBlobAllocator { 1024u };
};

} // namespace sts
