#pragma once

#include <sqee/assert.hpp>
#include <sqee/builtins.hpp>

#include "render/SceneData.hpp"

#include "game/Blobs.hpp"
#include "game/ParticleSet.hpp"

namespace sts {

//============================================================================//

struct LocalDiamond { float offsetTop, offsetMiddle, halfWidth; };

struct WorldDiamond
{
    float negX, posX, negY, posY, crossX, crossY;

    Vec2F neg_x() const { return { negX, crossY }; }
    Vec2F pos_x() const { return { posX, crossY }; }
    Vec2F neg_y() const { return { crossX, negY }; }
    Vec2F pos_y() const { return { crossX, posY }; }

    Vec2F origin() const { return { crossX, negY }; }
    Vec2F centre() const { return { crossX, crossY }; }

    WorldDiamond translated(Vec2F vec) const
    {
        return { negX + vec.x, posX + vec.x, negY + vec.y, posY + vec.y,
                 crossX + vec.x, crossY + vec.y };
    }
};

//============================================================================//

class FightWorld final : sq::NonCopyable
{
public: //====================================================//

    FightWorld();

    ~FightWorld();

    //--------------------------------------------------------//

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
    sq::PoolAllocator<HitBlob>& get_hit_blob_allocator();

    /// Access the HurtBlob Allocator.
    sq::PoolAllocator<HurtBlob>& get_hurt_blob_allocator();

    /// Access the enabled HitBlobs.
    const std::vector<HitBlob*>& get_hit_blobs() const;

    /// Access the enabled HurtBlobs.
    const std::vector<HurtBlob*>& get_hurt_blobs() const;

    //--------------------------------------------------------//

    /// Access the ParticleSystem.
    ParticleSet& get_particle_set() { return mParticleSet; }

    //--------------------------------------------------------//

    SceneData compute_scene_data() const;

private: //===================================================//

    unique_ptr<Stage> mStage;

    std::array<unique_ptr<Fighter>, 4> mFighters;

    ParticleSet mParticleSet;

    //--------------------------------------------------------//

    friend class PrivateWorld;
    unique_ptr<PrivateWorld> impl;
};

//============================================================================//

} // namespace sts
