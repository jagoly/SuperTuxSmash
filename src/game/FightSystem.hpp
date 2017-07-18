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

    /// Create a new hit blob attached to a fighter and an action.
    HitBlob* create_hit_blob(HitBlob::Type type, Fighter& fighter, Action& action);

    /// Create a new hit blob attached to a fighter only.
    HitBlob* create_hit_blob(HitBlob::Type type, Fighter& fighter);

    /// Delete an existing hit blob.
    void delete_hit_blob(HitBlob* blob);

    //--------------------------------------------------------//

    const auto& get_offensive_blobs() const { return mOffensiveBlobs; }
    const auto& get_damageable_blobs() const { return mDamageableBlobs; }

private: //===================================================//

    std::vector<HitBlob*> mOffensiveBlobs;
    std::vector<HitBlob*> mDamageableBlobs;

    //--------------------------------------------------------//

    struct BlobAllocator
    {
        BlobAllocator(uint maxBlobs);

        HitBlob* allocate();

        void deallocate(HitBlob* ptr);

        struct alignas(64) FreeSlot
        {
            FreeSlot* nextFreeSlot;
            uint8_t padding[56];
        };

        unique_ptr<FreeSlot[]> storage;

        FreeSlot* nextFreeSlot;
        FreeSlot* finalSlot;
    };

    //--------------------------------------------------------//

    BlobAllocator mBlobAllocator { 1024u };
};

} // namespace sts
