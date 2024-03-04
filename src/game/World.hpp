#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include <sqee/app/WrenPlus.hpp>

#include <random> // mt19937

namespace sts {

//============================================================================//

class World final
{
public: //====================================================//

    World(const Options& options, sq::AudioContext& audio, ResourceCaches& caches, Renderer& renderer);

    SQEE_COPY_DELETE(World)
    SQEE_MOVE_DELETE(World)

    ~World();

    //--------------------------------------------------------//

    const Options& options;

    sq::AudioContext& audio;

    ResourceCaches& caches;

    Renderer& renderer;

    //--------------------------------------------------------//

    /// Data that is only relevant to the editor.
    struct EditorData
    {
        String ctxKey;
        String errorMessage;
    };

    std::unique_ptr<EditorData> editor;

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
        WrenHandle* article_do_updates = nullptr;
        WrenHandle* article_do_destroy = nullptr;
    } handles;

    //--------------------------------------------------------//

    void tick();

    void integrate(float blend);

    //--------------------------------------------------------//

    /// Generate a new entity id.
    int32_t generate_entity_id() { return ++mEntityId; }

    /// Set the stage for the game.
    Stage& create_stage(TinyString name);

    /// Add a fighter to the game.
    Fighter& create_fighter(TinyString name);

    /// Load an article definition for later.
    ArticleDef& load_article_def(const String& path);

    /// Create a new article from a definition.
    Article& create_article(const ArticleDef& def, Fighter* fighter);

    /// Called after the stage and fighters have been added.
    void finish_setup();

    //--------------------------------------------------------//

    /// Compute a bounding box around all fighters.
    MinMax<Vec2F> compute_fighter_bounds() const;

    /// Get a list of all fighters sorted by update order.
    StackVector<Fighter*, MAX_FIGHTERS> get_sorted_fighters() const;

    //--------------------------------------------------------//

    Stage& get_stage() { return *mStage; }

    const Stage& get_stage() const { return *mStage; }

    const StackVector<std::unique_ptr<Fighter>, MAX_FIGHTERS>& get_fighters() const { return mFighters; }

    const std::vector<std::unique_ptr<Article>>& get_articles() const { return mArticles; }

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

    void wren_cancel_sound(int32_t id);

    void wren_cancel_effect(int32_t id);

private: //===================================================//

    void impl_update_collisions();

    //--------------------------------------------------------//

    std::unique_ptr<EffectSystem> mEffectSystem;

    std::unique_ptr<ParticleSystem> mParticleSystem;

    std::unique_ptr<Stage> mStage;

    StackVector<std::unique_ptr<Fighter>, MAX_FIGHTERS> mFighters;

    std::vector<std::unique_ptr<Article>> mArticles;

    // loaded fighter definitions, by name
    std::map<TinyString, FighterDef> mFighterDefs;

    // loaded article definitions, by path
    std::map<String, ArticleDef> mArticleDefs;

    int32_t mEntityId = -1;

    // at the end of the structure, because it's huge
    std::mt19937 mRandNumGen;

    friend EditorScene;
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::World)

WRENPLUS_BASE_CLASS_HEADER(sts::Entity)
