#pragma once

#include "setup.hpp"

#include "caches/SoundCache.hpp"

#include <sqee/app/WrenPlus.hpp>

namespace sts {

//============================================================================//

class FightWorld final : sq::NonCopyable
{
public: //====================================================//

    FightWorld(const Options& options, sq::AudioContext& audio);

    ~FightWorld();

    //--------------------------------------------------------//

    const Options& options;

    sq::AudioContext& audio;

    SoundCache sounds;

    wren::WrenPlusVM vm;

    struct {
        WrenHandle* script_new = nullptr;
        WrenHandle* script_reset = nullptr;
        WrenHandle* script_cancel = nullptr;
        WrenHandle* fiber_call = nullptr;
        WrenHandle* fiber_isDone = nullptr;
    } handles;

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

    /// Enable a hitblob.
    void enable_hitblob(HitBlob* blob);

    /// Disable a hitblob.
    void disable_hitblob(HitBlob* blob);

    /// Disable an action's hitblobs.
    void disable_hitblobs(const Action& action);

    /// Disable all hitblobs.
    void editor_clear_hitblobs();

    //--------------------------------------------------------//

    /// Enable a hurtblob.
    void enable_hurtblob(HurtBlob* blob);

    /// Disable a hurtblob.
    void disable_hurtblob(HurtBlob* blob);

    /// Disable a fighter's hurtblobs.
    void disable_hurtblobs(const Fighter& fighter);

    /// Disable all hurtblobs.
    void editor_clear_hurtblobs();

    //--------------------------------------------------------//

    /// Reset a fighter's collision group.
    void reset_collisions(uint8_t fighter, uint8_t group);

    /// Reset all of a fighter's collision groups.
    void reset_all_collisions(uint8_t fighter);

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
    //auto get_hurt_blob_allocator() { return mHurtBlobAlloc.get(); }

    /// Access the HitBlob Allocator.
    //auto get_hit_blob_allocator() { return mHitBlobAlloc.get(); }

    /// Access the Emitter Allocator.
    //auto get_emitter_allocator() { return mEmitterAlloc.get(); }

    /// Access the SoundEffect Allocator.
    //auto get_sound_effect_allocator() { return mSoundEffectAlloc.get(); }

    /// Access the polymorphic memory resource.
    std::pmr::memory_resource* get_memory_resource();

    /// Access the enabled HurtBlobs.
    const std::vector<HurtBlob*>& get_hurt_blobs() const { return mEnabledHurtBlobs; };

    /// Access the enabled HitBlobs.
    const std::vector<HitBlob*>& get_hit_blobs() const { return mEnabledHitBlobs; };

    //--------------------------------------------------------//

    /// Access this world's ParticleSystem.
    ParticleSystem& get_particle_system() { return *mParticleSystem; }

private: //===================================================//

    // undo stack in the editor can cause us to run out of slots, so we reserve more space
    // todo: for the editor, use big pools shared between all contexts

    //sq::PoolAllocatorStore<std::pair<const TinyString, HurtBlob>> mHurtBlobAlloc;
    //sq::PoolAllocatorStore<std::pair<const TinyString, HitBlob>> mHitBlobAlloc;
    //sq::PoolAllocatorStore<std::pair<const TinyString, Emitter>> mEmitterAlloc;
    //sq::PoolAllocatorStore<std::pair<const TinyString, SoundEffect>> mSoundEffectAlloc;

    std::unique_ptr<std::byte[]> mMemoryBuffer;
    std::unique_ptr<std::pmr::monotonic_buffer_resource> mMemoryResource;

    //--------------------------------------------------------//

    void impl_update_collisions();

    //--------------------------------------------------------//

    std::unique_ptr<ParticleSystem> mParticleSystem;

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
};

//============================================================================//

} // namespace sts
