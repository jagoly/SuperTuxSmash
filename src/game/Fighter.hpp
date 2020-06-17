#pragma once

#include <sqee/misc/Builtins.hpp>

#include <sqee/render/Armature.hpp>

#include "game/FightWorld.hpp"
#include "game/Actions.hpp"

namespace sts {

//============================================================================//

class Fighter : sq::NonCopyable
{
public: //====================================================//

    enum class State
    {
        Neutral, Walking, Dashing, Brake, Crouch,
        Charge, Attack, Special, Landing, PreJump, Jumping,
        Falling, AirAttack, AirSpecial, Knocked, Stunned,
        Shield, Dodge, EvadeBack, EvadeForward, AirDodge,
        LedgeHang, LedgeClimb,
        EditorPreview
    };

    enum class Facing
    {
        Left = -1, Right = +1
    };

    enum class AnimMode
    {
        Standard,    ///< non-looping, without root motion
        //Looping,     ///< looping, without root motion
        ApplyMotion, ///< non-looping, apply root motion continuously
        FixedMotion, ///< non-looping, apply root motion at end of animation
        WalkCycle,   ///< looping, update using velocity and anim_walk_stride
        DashCycle,   ///< looping, update using velocity and anim_dash_stride
        JumpAscend,  ///< non-looping, update using difference from jump velocity
        BrakeSlow,   ///< non-looping, update using difference from brake velocity
        //AboutFace,   ///< non-looping, change facing at end of animation
        Manual       ///< used by the editor
    };

    struct Animation
    {
        sq::Armature::Animation anim;
        AnimMode mode;
    };

    struct Transition
    {
        State newState;
        uint fadeFrames;
        const Animation* animation;
        const Animation* loop;
    };

    //--------------------------------------------------------//

    struct Stats
    {
        float walk_speed     = 1.f;
        float dash_speed     = 1.f;
        float air_speed      = 1.f;
        float traction       = 1.f;
        float air_mobility   = 1.f;
        float air_friction   = 1.f;
        float hop_height     = 1.f;
        float jump_height    = 1.f;
        float air_hop_height = 1.f;
        float gravity        = 1.f;
        float fall_speed     = 1.f;
        float weight         = 1.f;

        uint dodge_finish     = 22u;
        uint dodge_safe_start = 2u;
        uint dodge_safe_end   = 14u;

        uint evade_back_finish     = 24u;
        uint evade_back_safe_start = 3u;
        uint evade_back_safe_end   = 13u;

        uint evade_forward_finish     = 24u;
        uint evade_forward_safe_start = 3u;
        uint evade_forward_safe_end   = 13u;

        uint air_dodge_finish     = 26u;
        uint air_dodge_safe_start = 2u;
        uint air_dodge_safe_end   = 22u;

        uint ledge_climb_finish = 34u;

        float anim_walk_stride = 2.f;
        float anim_dash_stride = 3.f;

        //float anim_evade_distance       = 2.f;
        //float anim_ledge_climb_distance = 0.5f;
    };

    //--------------------------------------------------------//

    struct Status
    {
        bool intangible = false;
        uint timeSinceLedge = 0u;
        float damage = 0.f;
        float shield = 0.f;
        struct Ledge* ledge = nullptr;
    };

    //--------------------------------------------------------//

    virtual ~Fighter();

    //--------------------------------------------------------//

    virtual void tick() = 0;

    //--------------------------------------------------------//

    const uint8_t index;

    const FighterEnum type;

    Stats stats;

    Status status;

    //--------------------------------------------------------//

    // all of the data edited by the editor should be here for now
    // in the future, may make them private and add getters

    Array<UniquePtr<Action>, sq::enum_count_v<ActionType>> actions;

    std::unordered_map<SmallString, Animation> animations;

    sq::PoolMap<TinyString, HurtBlob> hurtBlobs;

    LocalDiamond diamond;

    //--------------------------------------------------------//

    struct {

        State state = State::Neutral;
        Facing facing = Facing::Right;

        Action* action = nullptr;

    } current, previous;

    //--------------------------------------------------------//

    const sq::Armature& get_armature() const;

    //--------------------------------------------------------//

    // temporary, for debug

    void set_controller(Controller* controller);
    Controller* get_controller();

    //--------------------------------------------------------//

    Action* get_action(ActionType type);

    //Animation* get_animation(AnimationType type);

    //--------------------------------------------------------//

    /// Called when hit by a basic attack.
    void apply_hit_basic(const HitBlob& hit);

    /// Called when passing the stage boundary.
    void pass_boundary();

    //-- access data needed for world updates ----------------//

    /// Get current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get current armature pose matrices.
    const Vector<Mat34F>& get_bone_matrices() const { return mBoneMatrices; }

    Vec2F get_position() const; ///< Get current position.

    Vec2F get_velocity() const; ///< Get current velocity.

    //-- compute data needed for rendering -------------------//

    /// Compute interpolated model matrix.
    Mat4F interpolate_model_matrix(float blend) const;

    /// Compute interpolated armature pose matrices.
    void interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const;

    //-- interface used by ActionFuncs -----------------------//

    Vec2F& edit_position();

    Vec2F& edit_velocity();

protected: //=================================================//

    Fighter(uint8_t index, FightWorld& world, FighterEnum type);

    //--------------------------------------------------------//

    FightWorld& mFightWorld;

    Vec2F mVelocity = { 0.f, 0.f };
    Vec2F mTranslate = { 0.f, 0.f };

    //--------------------------------------------------------//

    void base_tick_fighter();

    void base_tick_animation();

private: //===================================================//

    Vector<Mat34F> mBoneMatrices;

    Mat4F mModelMatrix;

    //--------------------------------------------------------//

    friend class PrivateFighter;
    UniquePtr<PrivateFighter> impl;

public: //== debug and editor interfaces =====================//

    void debug_show_fighter_widget();

    void debug_reload_actions();

    PrivateFighter* editor_get_private() { return impl.get(); }
};

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::Fighter::State,
                 Neutral, Walking, Dashing, Brake, Crouch,
                 Charge, Attack, Special, Landing, PreJump, Jumping,
                 Falling, AirAttack, AirSpecial, Knocked, Stunned,
                 Shield, Dodge, EvadeBack, EvadeForward, AirDodge,
                 LedgeHang, LedgeClimb,
                 EditorPreview)
