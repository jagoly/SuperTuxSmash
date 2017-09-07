#pragma once

#include <sqee/builtins.hpp>

#include <sqee/render/Armature.hpp>

#include "game/Controller.hpp"
#include "game/FightWorld.hpp"
#include "game/Actions.hpp"

//============================================================================//

namespace sts {

class Fighter : sq::NonCopyable
{
public: //====================================================//

    using Armature = sq::Armature;

    //--------------------------------------------------------//

    struct Stats
    {
        float walk_speed      = 1.f;
        float dash_speed      = 1.f;
        float air_speed       = 1.f;
        float traction        = 1.f;
        float air_mobility    = 1.f;
        float air_friction    = 1.f;
        float hop_height      = 1.f;
        float jump_height     = 1.f;
        float air_hop_height  = 1.f;
        float gravity         = 1.f;
        float fall_speed      = 1.f;
    };

    //--------------------------------------------------------//

    enum class State
    {
        Neutral,
        Walking,
        Dashing,
        Brake,
        Crouch,
        Attack,
        Landing,
        PreJump,
        Jumping,
        Falling,
        AirAttack,
        Knocked,
        Stunned,
    };

    enum class Facing
    {
        Left = -1,
        Right = +1
    };

    State state = State::Neutral;
    Facing facing = Facing::Right;

    //--------------------------------------------------------//

    struct Animations
    {
        Armature::Animation crouch_loop;
        Armature::Animation dashing_loop;
        Armature::Animation falling_loop;
        Armature::Animation jumping_loop;
        Armature::Animation neutral_loop;
        Armature::Animation walking_loop;

        Armature::Animation airhop;
        Armature::Animation brake;
        Armature::Animation crouch;
        Armature::Animation jump;
        Armature::Animation land;
        Armature::Animation stand;

        //Armature::Animation knocked;

        Armature::Animation action_Neutral_First;

        Armature::Animation action_Tilt_Down;
        Armature::Animation action_Tilt_Forward;
        Armature::Animation action_Tilt_Up;

        Armature::Animation action_Air_Back;
        Armature::Animation action_Air_Down;
        Armature::Animation action_Air_Forward;
        Armature::Animation action_Air_Neutral;
        Armature::Animation action_Air_Up;

        Armature::Animation action_Dash_Attack;
    };

    struct Transition
    {
        State newState; uint fadeFrames;
        const Armature::Animation* animation;
        const Armature::Animation* loop;
    };

    struct Transitions
    {
        Transition neutral_crouch;
        Transition neutral_jump;
        Transition neutral_walking;

        Transition walking_crouch;
        Transition walking_jump;
        Transition walking_dashing;
        Transition walking_neutral;

        Transition dashing_jump;
        Transition dashing_brake;

        Transition crouch_jump;
        Transition crouch_stand;

        Transition jumping_hop;
        Transition jumping_land;
        Transition jumping_fall;

        Transition falling_hop;
        Transition falling_land;

        Transition attack_to_neutral;
        Transition attack_to_crouch;
        Transition attack_to_falling;
    };

    //--------------------------------------------------------//

    Fighter(uint8_t index, FightWorld& world, Controller& controller, string path);

    virtual ~Fighter();

    //--------------------------------------------------------//

    virtual void tick() = 0;

    //--------------------------------------------------------//

    const uint8_t index;

    Stats stats;

    Animations animations;

    Transitions transitions;

    //--------------------------------------------------------//

    /// Called when hit by a basic attack.
    void apply_hit_basic(const HitBlob& hit);

    //--------------------------------------------------------//

    /// Access the active animation, or nullptr.
    const Armature::Animation* get_animation() const { return mAnimation; }

    //--------------------------------------------------------//

    /// Begin playing a different animation.
    ///
    /// @param animation The Animation to switch to.
    /// @param fadeFrames Number of frames to cross-fade for.

    void play_animation(const Armature::Animation& animation, uint fadeFrames);

    //--------------------------------------------------------//

    /// Get current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get current armature pose matrices.
    const std::vector<Mat34F>& get_bone_matrices() const { return mBoneMatrices; }

    /// Compute interpolated model matrix.
    Mat4F interpolate_model_matrix(float blend) const;

    /// Compute interpolated armature pose matrices.
    std::vector<Mat34F> interpolate_bone_matrices(float blend) const;

    //--------------------------------------------------------//

    // todo: make better interface for actions to use

    Vec2F mVelocity = { 0.f, 0.f };

protected: //=================================================//

    unique_ptr<Actions> mActions;

    Armature mArmature;

    std::vector<HurtBlob*> mHurtBlobs;

    //--------------------------------------------------------//

    FightWorld& mFightWorld;

    Controller& mController;

    bool mAttackHeld = false;
    bool mJumpHeld = false;

    PhysicsDiamond mLocalDiamond, mWorldDiamond;

    //--------------------------------------------------------//

    void base_tick_fighter();

    void base_tick_animation();

private: //===================================================//

    struct InterpolationData
    {
        Vec2F position { 0.f, 1.f };
        sq::Armature::Pose pose;
    }
    previous, current;

    //--------------------------------------------------------//

    uint mLandingLag = 0u;
    uint mJumpDelay = 0u;

    //--------------------------------------------------------//

    const Armature::Animation* mAnimation = nullptr;
    const Armature::Animation* mNextAnimation = nullptr;

    int mAnimTimeDiscrete = 0;
    float mAnimTimeContinuous = 0.f;

    Armature::Pose mFadeStartPose;

    uint mFadeFrames = 0u;
    uint mFadeProgress = 0u;

    //--------------------------------------------------------//

    Action::Type mActionType = Action::Type::None;
    Action* mActiveAction = nullptr;

    //--------------------------------------------------------//

    std::vector<Mat34F> mBoneMatrices;

    Mat4F mModelMatrix;

    //--------------------------------------------------------//

    void impl_initialise_armature(const string& path);

    void impl_initialise_hurt_blobs(const string& path);

    void impl_initialise_stats(const string& path);

    //--------------------------------------------------------//

    void impl_handle_input_movement(const Controller::Input& input);

    void impl_handle_input_actions(const Controller::Input& input);

    //--------------------------------------------------------//

    void impl_state_transition(const Transition& transition);

    void impl_switch_action(Action::Type actionType);

    //--------------------------------------------------------//

    void impl_update_physics();

    void impl_update_active_action();
};

//============================================================================//

} // namespace sts
