#pragma once

#include <sqee/misc/Builtins.hpp>

#include <sqee/misc/PoolTools.hpp>

#include "render/SceneData.hpp"

#include "game/Blobs.hpp"
#include "game/ParticleSystem.hpp"
#include "game/ParticleEmitter.hpp"

//============================================================================//

namespace sts {

class PrivateWorld final : sq::NonCopyable
{
public: //====================================================//

    PrivateWorld(FightWorld& world) : world(world) {}

    //--------------------------------------------------------//

    void tick();

    //--------------------------------------------------------//

    /// Set the stage for the game.
    void set_stage(UniquePtr<Stage> stage);

    /// Add a fighter to the game.
    void add_fighter(UniquePtr<Fighter> fighter);

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
    Vector<Fighter*> get_fighters()
    {
        Vector<Fighter*> result;
        result.reserve(4u);

        for (auto& uptr : mFighters)
            if (uptr != nullptr)
                result.push_back(uptr.get());

        return result;
    }

    //--------------------------------------------------------//

    SceneData compute_scene_data() const;

    //--------------------------------------------------------//

    sq::PoolAllocator<HitBlob> hitBlobAlloc { 1024u };
    sq::PoolAllocator<HurtBlob> hurtBlobAlloc { 128u };

    sq::PoolAllocator<ParticleEmitter> emitterAlloc { 1024u };

    Vector<HitBlob*> enabledHitBlobs;
    Vector<HurtBlob*> enabledHurtBlobs;

    Array<Array<uint32_t, 4>, 4> hitBitsArray;

private: //===================================================//

    UniquePtr<Stage> mStage;

    Array<UniquePtr<Fighter>, 4> mFighters;

    //--------------------------------------------------------//

    struct Collision { HitBlob* hit; HurtBlob* hurt; };

    Array<Array<Vector<Collision>, 4u>, 4u> mCollisions;

    //--------------------------------------------------------//

    FightWorld& world;
};

} // namespace sts
