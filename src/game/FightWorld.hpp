#pragma once

#include "setup.hpp"

#include "Blobs.hpp"
#include "Emitter.hpp"

#include <sqee/redist/sol_fwd.hpp>

namespace sts {

//============================================================================//

class FightWorld final : sq::NonCopyable
{
public: //====================================================//

    FightWorld(const Options& options);

    ~FightWorld();

    //--------------------------------------------------------//

    const Options& options;

    //--------------------------------------------------------//

    void tick();

    //--------------------------------------------------------//

    /// Set the stage for the game.
    void set_stage(std::unique_ptr<Stage> stage);

    /// Add a fighter to the game.
    void add_fighter(std::unique_ptr<Fighter> fighter);

    /// Called after the stage and fighters have been added.
    void finish_setup();

    //--------------------------------------------------------//

    /// Enable a hit blob.
    void enable_hit_blob(HitBlob* blob);

    /// Disable a hit blob.
    void disable_hit_blob(HitBlob* blob);

    /// Enable a hurt blob.
    void enable_hurt_blob(HurtBlob* blob);

    /// Disable a hurt blob.
    void disable_hurt_blob(HurtBlob* blob);

    //--------------------------------------------------------//

    /// Reset all of a fighter's hit blob groups.
    void reset_all_hit_blob_groups(Fighter& fighter);

    /// Disable all of a fighter's hit blobs.
    void disable_all_hit_blobs(Fighter& fighter);

    /// Disable all of a fighter's hurt blobs.
    void disable_all_hurt_blobs(Fighter& fighter);

    //--------------------------------------------------------//

    /// Access the stage.
    Stage& get_stage() { return *mStage; }

    /// Access the stage (const).
    const Stage& get_stage() const { return *mStage; }

    /// Access a fighter by index, might be null.
    Fighter* get_fighter(uint8_t index) { return mFighters[index].get(); }

    /// Access a fighter by index, might be null (const).
    const Fighter* get_fighter(uint8_t index) const { return mFighters[index].get(); }

    /// Acquire an iterable of all added fighters.
    auto get_fighters() { return sq::StackVector<Fighter*, 4>(mFighterRefs.begin(), mFighterRefs.end()); }

    /// Acquire an iterable of all added fighters (const).
    auto get_fighters() const { return sq::StackVector<const Fighter*, 4>(mFighterRefs.begin(), mFighterRefs.end()); }

    //--------------------------------------------------------//

    /// Access the HurtBlob Allocator.
    auto get_hurt_blob_allocator() { return mHurtBlobAlloc.get(); }

    /// Access the HitBlob Allocator.
    auto get_hit_blob_allocator() { return mHitBlobAlloc.get(); }

    /// Access the Emitter Allocator.
    auto get_emitter_allocator() { return mEmitterAlloc.get(); }

    /// Access the enabled HurtBlobs.
    const std::vector<HurtBlob*>& get_hurt_blobs() const { return mEnabledHurtBlobs; };

    /// Access the enabled HitBlobs.
    const std::vector<HitBlob*>& get_hit_blobs() const { return mEnabledHitBlobs; };

    //--------------------------------------------------------//

    /// Access the shared Lua State.
    sol::state& get_lua_state() { return *mLuaState; }

    /// Access the ParticleSystem.
    ParticleSystem& get_particle_system() { return *mParticleSystem; }

private: //===================================================//

    // undo stack in the editor can cause us to run out of slots, so we reserve more space
    // todo: for the editor, use big pools shared between all contexts

    sq::PoolAllocatorStore<std::pair<const TinyString, HurtBlob>> mHurtBlobAlloc;
    sq::PoolAllocatorStore<std::pair<const TinyString, HitBlob>> mHitBlobAlloc;
    sq::PoolAllocatorStore<std::pair<const TinyString, Emitter>> mEmitterAlloc;

    //--------------------------------------------------------//

    void impl_update_collisions();

    //--------------------------------------------------------//

    std::unique_ptr<sol::state> mLuaState;

    std::unique_ptr<ParticleSystem> mParticleSystem;

    //--------------------------------------------------------//

    std::unique_ptr<Stage> mStage;

    std::array<std::unique_ptr<Fighter>, MAX_FIGHTERS> mFighters;
    sq::StackVector<Fighter*, MAX_FIGHTERS> mFighterRefs;

    std::vector<HitBlob*> mEnabledHitBlobs;
    std::vector<HurtBlob*> mEnabledHurtBlobs;

    //--------------------------------------------------------//

    struct Collision { HitBlob& hit; HurtBlob& hurt; };

    // turns out you can do value-aggregate initialisation with nested std arrays
    std::array<std::array<std::array<bool, MAX_FIGHTERS>, MAX_HITBLOB_GROUPS>, MAX_FIGHTERS> mHitBlobGroups {};

    std::array<std::vector<Collision>, MAX_FIGHTERS> mCollisions;

public: //== debug and editor interfaces =====================//

    void debug_reload_actions();

    void editor_disable_all_hurtblobs();
};

//============================================================================//

} // namespace sts
