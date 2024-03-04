#pragma once

#include "setup.hpp"

#include "game/Entity.hpp"
#include "game/FighterDef.hpp"
#include "game/Physics.hpp"

namespace sts {

//============================================================================//

class Fighter final : public Entity
{
public: //====================================================//

    /// Describes when to stop at the edge of a platform.
    enum class EdgeStopMode : uint8_t
    {
        Never,  ///< Never stop at edges.
        Always, ///< Always stop at edges
        Input,  ///< Stop if axis not pushed.
    };

    using Attributes = FighterDef::Attributes;

    //--------------------------------------------------------//

    /// Public information and variables for fighters.
    struct Variables : EntityVars
    {
        uint8_t extraJumps = 0u;
        uint8_t lightLandTime = 0u;
        uint8_t noCatchTime = 0u;
        uint8_t stunTime = 0u;
        uint8_t reboundTime = 0u;

        EdgeStopMode edgeStop = {};

        bool intangible = false;
        bool invincible = false;
        bool fastFall = false;
        bool applyGravity = true;
        bool applyFriction = true;
        bool flinch = false;
        bool grabable = true;

        bool onGround = true;
        bool onPlatform = false;

        int8_t edge = 0;

        float moveMobility = 0.f;
        float moveSpeed = 0.f;

        float damage = 0.f;
        float shield = SHIELD_MAX_HP;

        float launchSpeed = 0.f;
        int32_t launchEntity = -1;

        Ledge* ledge = nullptr;
    };

    //--------------------------------------------------------//

    Fighter(const FighterDef& def, uint8_t index);

    ~Fighter();

    //--------------------------------------------------------//

    const FighterDef& def;

    const uint8_t index;

    //--------------------------------------------------------//

    Attributes attributes;

    Variables variables;

    Diamond diamond;

    Controller* controller = nullptr;

    FighterAction* activeAction = nullptr;

    FighterState* activeState = nullptr;

    //--------------------------------------------------------//

    void tick();

    void integrate(float blend);

    //--------------------------------------------------------//

    /// Called for each hit, returns true if the attack was not ignored.
    bool accumulate_hit(const HitBlob& hit, const HurtBlob& hurt);

    /// Apply knockback, as if hit by a HitBlob.
    void apply_knockback(const HitBlobDef& hitDef, const Entity& hitEntity);

    /// Called after all hits have been accumulated.
    void apply_hits();

    /// Called when we sucessfully grab someone.
    void apply_grab(Fighter& victim);

    /// Called when our attack clashes with someone else's.
    void apply_rebound(float damage);

    /// Called when passing the stage boundary.
    void pass_boundary();

    /// Called at the end of a throw animation.
    void set_position_after_throw();

    /// Called at game start and when respawning.
    void set_spawn_transform(Vec2F position, int8_t facing);

    //--------------------------------------------------------//

    std::vector<HurtBlob>& get_hurt_blobs() { return mHurtBlobs; }

    const EntityDef& get_def() const override { return def; }

    EntityVars& get_vars() override { return variables; }

    const EntityVars& get_vars() const override { return variables; }

    //-- wren methods ----------------------------------------//

    WrenHandle* wren_get_library() { return mLibraryHandle; }

    void wren_log(StringView message);

    void wren_cxx_assign_action(SmallString key);

    void wren_cxx_assign_action_null() { clear_action(); }

    void wren_cxx_assign_state(TinyString key);

    Article* wren_cxx_spawn_article(TinyString key);

    bool wren_attempt_ledge_catch();

    void wren_enable_hurtblob(TinyString key);

    void wren_disable_hurtblob(TinyString key);

private: //===================================================//

    //-- init methods, called by constructor -----------------//

    void initialise_armature();

    void initialise_attributes();

    void initialise_hurtblobs();

    void initialise_actions();

    void initialise_states();

    //-- methods used internally or by the editor ------------//

    Diamond compute_diamond() const;

    void start_action(FighterAction& action);

    void cancel_action();

    void change_state(FighterState& state);

    void clear_action();

    void reset_everything();

    //-- update methods, called each tick --------------------//

    void update_movement();

    void update_misc();

    void update_action();

    void update_state();

    void update_jitter();

    //--------------------------------------------------------//

    std::vector<HurtBlob> mHurtBlobs;

    // todo: better node maps
    std::map<SmallString, FighterAction> mActions;
    std::map<TinyString, FighterState> mStates;

    WrenHandle* mLibraryHandle = nullptr;

    //--------------------------------------------------------//

    uint8_t mJitterCounter = 0u;

    std::optional<BlobRegion> mHurtRegion;

    //-- debug and editor stuff ------------------------------//

    //void* editorErrorCause = nullptr;

    FighterAction* editorStartAction = nullptr;
    Fighter* editorApplyGrab = nullptr;

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER
(
    sts::Fighter::EdgeStopMode,
    Never, Always, Input
)

WRENPLUS_TRAITS_HEADER(sts::Fighter::Attributes)
WRENPLUS_TRAITS_HEADER(sts::Fighter::Variables)
WRENPLUS_TRAITS_HEADER(sts::Fighter)
