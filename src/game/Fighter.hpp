#pragma once

#include <sqee/builtins.hpp>

#include <sqee/render/Armature.hpp>

#include "game/Controller.hpp"
#include "game/FightWorld.hpp"
#include "game/ParticleSet.hpp"
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

    Fighter(uint8_t index, FightWorld& world, string_view name);

    virtual ~Fighter();

    //--------------------------------------------------------//

    virtual void tick() = 0;

    //--------------------------------------------------------//

    const uint8_t index;

    State state = State::Neutral;

    Facing facing = Facing::Right;

    Stats stats;

    Animations animations; // todo: why public?

    Transitions transitions; // todo: why public?

    //--------------------------------------------------------//

    string_view get_name() const { return mName; }

    //--------------------------------------------------------//

    Controller* get_controller() { return mController; }

    void set_controller(Controller* controller) { mController = controller; }

    const Controller* get_controller() const { return mController; }

    //--------------------------------------------------------//

    sq::Armature& get_armature() { return mArmature; }

    Actions& get_actions() { return *mActions; }

    //--------------------------------------------------------//

    /// Called when hit by a basic attack.
    void apply_hit_basic(const HitBlob& hit);

    /// Called when passing the stage boundary.
    void pass_boundary();

    //--------------------------------------------------------//

    /// Begin playing a different animation.
    ///
    /// @param animation The Animation to switch to.
    /// @param fadeFrames Number of frames to cross-fade for.

    void play_animation(const Animation& animation, uint fadeFrames);

    //-- access data needed for world updates ----------------//

    /// Get current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get current armature pose matrices.
    const std::vector<Mat34F>& get_bone_matrices() const { return mBoneMatrices; }

    /// Get current velocity.
    const Vec2F& get_velocity() const { return mVelocity; }

    /// Get world space physics diamond.
    const PhysicsDiamond& get_diamond() const { return mWorldDiamond; }

    //-- compute data needed for rendering -------------------//

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

    bool mAttackHeld = false;
    bool mJumpHeld = false;

    PhysicsDiamond mLocalDiamond, mWorldDiamond;

    //--------------------------------------------------------//

    void base_tick_fighter();

    void base_tick_animation();

    //--------------------------------------------------------//

    string_view mName;

private: //===================================================//

    struct InterpolationData
    {
        Vec2F position { 0.f, 1.f };
        sq::Armature::Pose pose;
    }
    previous, current;

    //--------------------------------------------------------//

    Controller* mController = nullptr;

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

    //-- init functions, called by constructor ---------------//

    void impl_initialise_armature(const string& path);

    void impl_initialise_hurt_blobs(const string& path);

    void impl_initialise_stats(const string& path);

    //-- should be the only places input is handled ----------//

    void impl_handle_input_movement(const Controller::Input& input);

    void impl_handle_input_actions(const Controller::Input& input);

    //--------------------------------------------------------//

    void impl_state_transition(const Transition& transition);

    void impl_switch_action(Action::Type actionType);

    //--------------------------------------------------------//

    void impl_update_physics();

    void impl_update_active_action();

    //--------------------------------------------------------//

    friend class GameScene;
    friend class ActionsEditor;

    //--------------------------------------------------------//

    void impl_show_fighter_widget();
};

//============================================================================//

#define ETSC SQEE_ENUM_TO_STRING_CASE

SQEE_ENUM_TO_STRING_BLOCK_BEGIN(Fighter::State)
ETSC(Neutral) ETSC(Walking) ETSC(Dashing) ETSC(Brake) ETSC(Crouch)
ETSC(Charge) ETSC(Attack) ETSC(Landing) ETSC(PreJump) ETSC(Jumping)
ETSC(Falling) ETSC(AirAttack) ETSC(Knocked) ETSC(Stunned)
SQEE_ENUM_TO_STRING_BLOCK_END

SQEE_ENUM_TO_STRING_BLOCK_BEGIN(Fighter::Facing)
ETSC(Left) ETSC(Right)
SQEE_ENUM_TO_STRING_BLOCK_END

#undef ETSC

//============================================================================//

} // namespace sts
