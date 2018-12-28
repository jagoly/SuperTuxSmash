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
        Shield, Dodge, Evade, AirDodge
    };

    enum class Facing
    {
        Left = -1, Right = +1
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
        float evade_distance = 1.f;

        uint dodge_finish     = 22u;
        uint dodge_safe_start = 2u;
        uint dodge_safe_end   = 14u;

        uint evade_finish     = 24u;
        uint evade_safe_start = 3u;
        uint evade_safe_end   = 13u;

        uint air_dodge_finish     = 26u;
        uint air_dodge_safe_start = 2u;
        uint air_dodge_safe_end   = 22u;
    };

    //--------------------------------------------------------//

    struct Actions
    {
        UniquePtr<Action> neutral_first;

        UniquePtr<Action> tilt_down;
        UniquePtr<Action> tilt_forward;
        UniquePtr<Action> tilt_up;

        UniquePtr<Action> air_back;
        UniquePtr<Action> air_down;
        UniquePtr<Action> air_forward;
        UniquePtr<Action> air_neutral;
        UniquePtr<Action> air_up;

        UniquePtr<Action> dash_attack;

        UniquePtr<Action> smash_down;
        UniquePtr<Action> smash_forward;
        UniquePtr<Action> smash_up;

        UniquePtr<Action> special_down;
        UniquePtr<Action> special_forward;
        UniquePtr<Action> special_neutral;
        UniquePtr<Action> special_up;
    };

    //--------------------------------------------------------//

    struct Status
    {
        float damage = 0.f;
        bool intangible = false;
        float shield = 0.f;
    };

    //--------------------------------------------------------//

    Fighter(uint8_t index, FightWorld& world, StringView name);

    virtual ~Fighter();

    //--------------------------------------------------------//

    virtual void tick() = 0;

    //--------------------------------------------------------//

    const uint8_t index;

    Stats stats;

    Status status;

    Actions actions;

    //--------------------------------------------------------//

    struct {

        State state = State::Neutral;
        Facing facing = Facing::Right;

        Action* action = nullptr;

    } current, previous;

    //--------------------------------------------------------//

    StringView get_name() const { return mName; }

    const sq::Armature& get_armature() const;

    //--------------------------------------------------------//

    // temporary, for debug

    void set_controller(Controller* controller);
    Controller* get_controller();

    //--------------------------------------------------------//

    Action* get_action(ActionType type);

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

    /// Get current velocity.
    const Vec2F& get_velocity() const { return mVelocity; }

    /// Get world space physics diamond.
    const WorldDiamond& get_diamond() const { return mWorldDiamond; }

    //-- compute data needed for rendering -------------------//

    /// Compute interpolated model matrix.
    Mat4F interpolate_model_matrix(float blend) const;

    /// Compute interpolated armature pose matrices.
    void interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const;

    //--------------------------------------------------------//

    // todo: make better interface for actions to use

    Vec2F mVelocity = { 0.f, 0.f };

protected: //=================================================//

    Vector<HurtBlob*> mHurtBlobs;

    //--------------------------------------------------------//

    FightWorld& mFightWorld;

    LocalDiamond mLocalDiamond;
    WorldDiamond mWorldDiamond;

    //--------------------------------------------------------//

    void base_tick_fighter();

    void base_tick_animation();

    //--------------------------------------------------------//

    StringView mName;

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
                 Shield, Dodge, Evade, AirDodge)
