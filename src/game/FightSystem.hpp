#pragma once

#include <sqee/builtins.hpp>

#include <sqee/misc/PoolTools.hpp>

#include "game/HitBlob.hpp"

//============================================================================//

namespace sts {

class FightWorld final : sq::NonCopyable
{
public: //====================================================//

    FightWorld();

    void tick();

    //--------------------------------------------------------//

    /// Add a fighter to the game.
    void add_fighter(unique_ptr<Fighter> fighter);

    //--------------------------------------------------------//

    /// Create a new hurt blob.
    HurtBlob* create_hurt_blob(Fighter& fighter);

    /// Delete an existing hurt blob.
    void delete_hurt_blob(HurtBlob* blob);

    //--------------------------------------------------------//

    /// Enable a hit blob.
    void enable_hit_blob(HitBlob* blob);

    /// Disable a hit blob.
    void disable_hit_blob(HitBlob* blob);

    //--------------------------------------------------------//

    /// Reset one of a fighter's hit blob groups.
    void reset_hit_blob_group(Fighter& fighter, uint8_t group);

    /// Update the sphere of an active hit blob.
    void update_hit_blob_sphere(HitBlob* blob, Vec3F origin, float radius);

    //--------------------------------------------------------//

    auto& get_hit_blob_allocator() { return mHitBlobAlloc; }

    //--------------------------------------------------------//

    const auto& get_hit_blobs() const { return mEnabledHitBlobs; }
    const auto& get_hurt_blobs() const { return mHurtBlobs; }

    //--------------------------------------------------------//

    std::pair<Vec2F, Vec2F> compute_camera_view_bounds() const;

private: //===================================================//

    std::array<unique_ptr<Fighter>, 4> mFighters;

    //--------------------------------------------------------//

    std::array<std::array<uint32_t, 4>, 4> mHitBitsArray;
    static_assert(sizeof(mHitBitsArray) == 64u);

    //--------------------------------------------------------//

    std::vector<HitBlob*> mEnabledHitBlobs;

    std::vector<HurtBlob*> mHurtBlobs;

    //--------------------------------------------------------//

    using Collision = std::pair<HitBlob*, HurtBlob*>;

    std::array<std::array<std::vector<Collision>, 4u>, 4u> mCollisions;

    //--------------------------------------------------------//

    sq::PoolAllocator<HitBlob> mHitBlobAlloc { 1024u };
    sq::PoolAllocator<HurtBlob> mHurtBlobAlloc { 128u };
};

} // namespace sts
