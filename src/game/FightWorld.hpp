#pragma once

#include <sqee/builtins.hpp>

#include <sqee/misc/PoolTools.hpp>

#include "render/SceneData.hpp"

#include "game/ParticleSet.hpp"
#include "game/Blobs.hpp"

//============================================================================//

namespace sts {

struct PhysicsDiamond
{
    Vec2F xNeg, xPos, yNeg, yPos;
    Vec2F centre() const { return (xNeg+xPos+yNeg+yPos)*0.25f; }
};

class FightWorld final : sq::NonCopyable
{
public: //====================================================//

    struct FighterStatus
    {
        uint32_t damage = 0u;
    };

    //--------------------------------------------------------//

    FightWorld();

    void tick();

    //--------------------------------------------------------//

    /// Set the stage for the game.
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
    Stage& get_stage() { SQASSERT(mStage, ""); return *mStage; }

    /// Access the stage (const).
    const Stage& get_stage() const { SQASSERT(mStage, ""); return *mStage; }

    /// Access a fighter by index, might be null.
    Fighter* get_fighter(uint8_t index) { return mFighters[index].get(); }

    /// Access a fighter by index, might be null (const).
    const Fighter* get_fighter(uint8_t index) const { return mFighters[index].get(); }

    //--------------------------------------------------------//

    /// Acquire a vector of all added fighters.
    std::vector<Fighter*> get_fighters()
    {
        std::vector<Fighter*> result;
        result.reserve(4u);

        for (auto& uptr : mFighters)
            if (uptr != nullptr)
                result.push_back(uptr.get());

        return result;
    }

    //--------------------------------------------------------//

    /// Access the HitBlob Allocator.
    auto& get_hit_blob_allocator() { return mHitBlobAlloc; }

    /// Access the HurtBlob Allocator.
    auto& get_hurt_blob_allocator() { return mHurtBlobAlloc; }

    //--------------------------------------------------------//

    const auto& get_hit_blobs() const { return mEnabledHitBlobs; }
    const auto& get_hurt_blobs() const { return mHurtBlobs; }

    //--------------------------------------------------------//

    SceneData compute_scene_data() const;

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
