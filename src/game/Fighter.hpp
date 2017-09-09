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

    enum class State
    {
        Neutral, Walking, Dashing, Brake, Crouch,
        Charge, Attack, Landing, PreJump, Jumping,
        Falling, AirAttack, Knocked, Stunned
    };

    enum class Facing
    {
        Left = -1, Right = +1
    };

    //--------------------------------------------------------//

    using Animation = sq::Armature::Animation;

    struct Transition
    {
        State newState; uint fadeFrames;
        const Animation* animation;
        const Animation* loop = nullptr;
    };

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

    struct Animations
    {
        Animation crouch_loop;
        Animation dashing_loop;
        Animation falling_loop;
        Animation jumping_loop;
        Animation neutral_loop;
        Animation walking_loop;

        Animation airhop;
        Animation brake;
        Animation crouch;
        Animation jump;
        Animation land;
        Animation stand;

        //Animation knocked;

        Animation action_neutral_first;

        Animation action_tilt_down;
        Animation action_tilt_forward;
        Animation action_tilt_up;

        Animation action_air_back;
        Animation action_air_down;
        Animation action_air_forward;
        Animation action_air_neutral;
        Animation action_air_up;

        Animation action_dash_attack;

        Animation action_smash_down_start;
        Animation action_smash_forward_start;
        Animation action_smash_up_start;

        Animation action_smash_down_charge;
        Animation action_smash_forward_charge;
        Animation action_smash_up_charge;

        Animation action_smash_down_attack;
        Animation action_smash_forward_attack;
        Animation action_smash_up_attack;
    };

    //--------------------------------------------------------//

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

        Transition other_fall;

        Transition smash_up_start;
        Transition smash_forward_start;
        Transition smash_down_start;

        Transition smash_up_attack;
        Transition smash_forward_attack;
        Transition smash_down_attack;

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

    State state = State::Neutral;

    Facing facing = Facing::Right;

    Stats stats;

    Animations animations;

    Transitions transitions;

    //--------------------------------------------------------//

    /// Called when hit by a basic attack.
    void apply_hit_basic(const HitBlob& hit);

    //--------------------------------------------------------//

    /// Access the active animation, or nullptr.
    const Animation* get_animation() const { return mAnimation; }

    //--------------------------------------------------------//

    /// Begin playing a different animation.
    ///
    /// @param animation The Animation to switch to.
    /// @param fadeFrames Number of frames to cross-fade for.

    void play_animation(const Animation& animation, uint fadeFrames);

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

    sq::Armature mArmature;

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

    const sq::Armature::Animation* mAnimation = nullptr;
    const sq::Armature::Animation* mNextAnimation = nullptr;
    const sq::Armature::Pose* mStaticPose = nullptr;

    int mAnimTimeDiscrete = 0;
    float mAnimTimeContinuous = 0.f;

    sq::Armature::Pose mFadeStartPose;

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
