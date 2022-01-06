#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include <sqee/app/WrenPlus.hpp>

#include <random> // mt19937

namespace sts {

//============================================================================//

class World final : sq::NonCopyable
{
public: //====================================================//

    World(bool editor, const Options& options, sq::AudioContext& audio, ResourceCaches& caches, Renderer& renderer);

    ~World();

    //--------------------------------------------------------//

    const bool editor;

    const Options& options;

    sq::AudioContext& audio;

    ResourceCaches& caches;

    Renderer& renderer;

    //--------------------------------------------------------//

    wren::WrenPlusVM vm;

    struct {
        WrenHandle* new_1 = nullptr;
        WrenHandle* action_do_start = nullptr;
        WrenHandle* action_do_updates = nullptr;
        WrenHandle* action_do_cancel = nullptr;
        WrenHandle* state_do_enter = nullptr;
        WrenHandle* state_do_updates = nullptr;
        WrenHandle* state_do_exit = nullptr;
    } handles;

    //--------------------------------------------------------//

    void tick();

    void integrate(float blend);

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
    void disable_hitblobs(const FighterAction& action);

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

    /// Reset a fighter's collisions.
    void reset_collisions(uint8_t fighter);

    /// Compute a bounding box around all fighters.
    MinMax<Vec2F> compute_fighter_bounds() const;

    //--------------------------------------------------------//

    /// Access the stage.
    Stage& get_stage() { return *mStage; }

    /// Access the stage (const).
    const Stage& get_stage() const { return *mStage; }

    /// Access a fighter by index.
    Fighter& get_fighter(uint8_t index) { return *mFighters[index]; }

    /// Access a fighter by index (const).
    const Fighter& get_fighter(uint8_t index) const { return *mFighters[index]; }

    //--------------------------------------------------------//

    // todo: in c++20 can use ranges to not expose the (unique) pointers

    /// Access an iterable of all fighters.
    const StackVector<std::unique_ptr<Fighter>, MAX_FIGHTERS>& get_fighters() const { return mFighters; }

    /// Access the enabled HurtBlobs.
    const std::vector<HurtBlob*>& get_hurt_blobs() const { return mEnabledHurtBlobs; };

    /// Access the enabled HitBlobs.
    const std::vector<HitBlob*>& get_hit_blobs() const { return mEnabledHitBlobs; };

    //--------------------------------------------------------//

    /// Access the random number generator.
    std::mt19937& get_rng() { return mRandNumGen; }

    /// Reset the random number generator seed.
    void set_rng_seed(uint_fast32_t seed) { mRandNumGen.seed(seed); }

    /// Access the EffectSystem.
    EffectSystem& get_effect_system() { return *mEffectSystem; }

    /// Access the ParticleSystem.
    ParticleSystem& get_particle_system() { return *mParticleSystem; }

    //-- wren methods ----------------------------------------//

    double wren_random_int(int min, int max);

    double wren_random_float(float min, float max);

private: //===================================================//

    void impl_update_collisions();

    //--------------------------------------------------------//

    std::mt19937 mRandNumGen;

    std::unique_ptr<EffectSystem> mEffectSystem;

    std::unique_ptr<ParticleSystem> mParticleSystem;

    std::unique_ptr<Stage> mStage;

    StackVector<std::unique_ptr<Fighter>, MAX_FIGHTERS> mFighters;

    std::vector<HitBlob*> mEnabledHitBlobs;
    std::vector<HurtBlob*> mEnabledHurtBlobs;

    //--------------------------------------------------------//

    struct Collision { HitBlob& hit; HurtBlob& hurt; };

    std::array<std::array<bool, MAX_FIGHTERS>, MAX_FIGHTERS> mIgnoreCollisions {};

    std::array<std::vector<Collision>, MAX_FIGHTERS> mCollisions;
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::World)
