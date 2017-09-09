#pragma once

#include <sqee/builtins.hpp>

#include <sqee/misc/PoolTools.hpp>

#include "game/Blobs.hpp"

//============================================================================//

namespace sts {

struct PhysicsDiamond
{
    Vec2F xNeg, xPos, yNeg, yPos;
};

class FightWorld final : sq::NonCopyable
{
public: //====================================================//

    FightWorld();

    void tick();

    //--------------------------------------------------------//

    /// Set the stage for them game.
    void set_stage(unique_ptr<Stage> stage);

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

    /// Reset all of a fighter's hit blob groups.
    void reset_all_hit_blob_groups(Fighter& fighter);

    /// Disable all of a fighter's hit blobs.
    void disable_all_hit_blobs(Fighter& fighter);

    //--------------------------------------------------------//

    /// Access the stage.
    Stage& get_stage() { return *mStage; }

    /// Access the stage (const).
    const Stage& get_stage() const { return *mStage; }

    //--------------------------------------------------------//

    /// Access the HitBlob Allocator.
    auto& get_hit_blob_allocator() { return mHitBlobAlloc; }

    /// Access the HurtBlob Allocator.
    auto& get_hurt_blob_allocator() { return mHurtBlobAlloc; }

    //--------------------------------------------------------//

    const auto& get_hit_blobs() const { return mEnabledHitBlobs; }
    const auto& get_hurt_blobs() const { return mHurtBlobs; }

    //--------------------------------------------------------//

    sq::maths::Sphere compute_camera_view_bounds() const;

private: //===================================================//

    unique_ptr<Stage> mStage;

    std::array<unique_ptr<Fighter>, 4> mFighters;

    //--------------------------------------------------------//

    std::array<std::array<uint32_t, 4>, 4> mHitBitsArray;

    //--------------------------------------------------------//

    std::vector<HitBlob*> mEnabledHitBlobs;

    std::vector<HurtBlob*> mHurtBlobs;

    //--------------------------------------------------------//

    struct Collision { HitBlob* hit; HurtBlob* hurt; };

    std::array<std::array<std::vector<Collision>, 4u>, 4u> mCollisions;

    //--------------------------------------------------------//

    sq::PoolAllocator<HitBlob> mHitBlobAlloc { 1024u };
    sq::PoolAllocator<HurtBlob> mHurtBlobAlloc { 128u };
};

} // namespace sts