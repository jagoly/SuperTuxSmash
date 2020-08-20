#pragma once

#include "setup.hpp" // IWYU pragma: export

#include "game/ActionEnums.hpp"
#include "game/FighterEnums.hpp"
#include "main/MainEnums.hpp"

#include <sqee/misc/PoolTools.hpp>
#include <sqee/objects/Armature.hpp>

//============================================================================//

namespace sts {

struct InputFrame;
struct Ledge;

//============================================================================//

struct LocalDiamond
{
    LocalDiamond() = default;

    LocalDiamond(float _halfWidth, float _offsetTop, float _offsetCross)
    {
        halfWidth = _halfWidth;
        offsetTop = _offsetTop;
        offsetCross = _offsetCross;

        normLeftDown = sq::maths::normalize(Vec2F(-_offsetCross, -_halfWidth));
        normLeftUp = sq::maths::normalize(Vec2F(-_offsetCross, +_halfWidth));
        normRightDown = sq::maths::normalize(Vec2F(+_offsetCross, -_halfWidth));
        normRightUp = sq::maths::normalize(Vec2F(+_offsetCross, +_halfWidth));
    }

    float halfWidth, offsetTop, offsetCross;
    Vec2F normLeftDown, normLeftUp, normRightDown, normRightUp;

    Vec2F min() const { return { -halfWidth, 0.f }; }
    Vec2F max() const { return { +halfWidth, offsetTop }; }
    Vec2F cross() const { return { 0.f, offsetCross }; }
};

//============================================================================//

class Fighter : sq::NonCopyable
{
public: //====================================================//

    using State = FighterState;
    using Command = FighterCommand;
    using AnimMode = FighterAnimMode;

    //--------------------------------------------------------//

    /// Structure containing an animation and related metadata.
    struct Animation
    {
        sq::Armature::Animation anim;
        AnimMode mode;
        String key;
    };

    /// Structure containing the basic animations used by a fighter.
    struct Animations
    {
        Animation DashingLoop;
        Animation FallingLoop;
        Animation NeutralLoop;
        Animation ProneLoop;
        Animation TumbleLoop;
        Animation VertigoLoop;
        Animation WalkingLoop;

        Animation ShieldOn;
        Animation ShieldOff;
        Animation ShieldLoop;

        Animation CrouchOn;
        Animation CrouchOff;
        Animation CrouchLoop;

        Animation DashStart;
        Animation VertigoStart;

        Animation Brake;
        Animation PreJump;
        Animation Turn;
        Animation TurnBrake;
        Animation TurnDash;

        Animation JumpBack;
        Animation JumpForward;
        Animation AirHopBack;
        Animation AirHopForward;

        Animation LedgeCatch;
        Animation LedgeLoop;
        Animation LedgeClimb;
        Animation LedgeJump;

        Animation NeutralFirst;

        Animation DashAttack;

        Animation TiltDown;
        Animation TiltForward;
        Animation TiltUp;

        Animation EvadeBack;
        Animation EvadeForward;
        Animation Dodge;

        Animation ProneAttack;
        Animation ProneBack;
        Animation ProneForward;
        Animation ProneStand;

        Animation SmashDownStart;
        Animation SmashForwardStart;
        Animation SmashUpStart;

        Animation SmashDownCharge;
        Animation SmashForwardCharge;
        Animation SmashUpCharge;

        Animation SmashDownAttack;
        Animation SmashForwardAttack;
        Animation SmashUpAttack;

        Animation AirBack;
        Animation AirDown;
        Animation AirForward;
        Animation AirNeutral;
        Animation AirUp;
        Animation AirDodge;

        Animation LandLight;
        Animation LandHeavy;
        Animation LandTumble;

        Animation LandAirBack;
        Animation LandAirDown;
        Animation LandAirForward;
        Animation LandAirNeutral;
        Animation LandAirUp;

        Animation HurtLowerLight;
        Animation HurtLowerHeavy;
        Animation HurtLowerTumble;

        Animation HurtMiddleLight;
        Animation HurtMiddleHeavy;
        Animation HurtMiddleTumble;

        Animation HurtUpperLight;
        Animation HurtUpperHeavy;
        Animation HurtUpperTumble;

        Animation HurtAirLight;
        Animation HurtAirHeavy;

        //Animation LaunchLoop;
        //Animation LaunchFinish;
    };

    /// Structure containing stats that generally don't change during a game.
    struct Stats
    {
        float walk_speed    = 0.1f;
        float dash_speed    = 0.15f;
        float air_speed     = 0.1f;
        float traction      = 0.005f;
        float air_mobility  = 0.008f;
        float air_friction  = 0.002f;
        float hop_height    = 1.5f;
        float jump_height   = 3.5f;
        float airhop_height = 3.5f;
        float gravity       = 0.01f;
        float fall_speed    = 0.15f;
        float weight        = 100.f;

        uint extra_jumps = 2u;

        uint land_heavy_min_time = 32u;

        uint dash_start_time  = 11u;
        uint dash_brake_time  = 12u;
        uint dash_turn_time   = 14u;

        float anim_walk_stride = 2.f;
        float anim_dash_stride = 3.f;
    };

    /// Structure containing the public state and status of a fighter.
    struct Status
    {
        State state = State::Neutral;
        int8_t facing = +1;
        Vec2F position = { 0.f, 0.f };
        Vec2F velocity = { 0.f, 0.f };
        bool intangible = false;
        bool autocancel = false;
        float damage = 0.f;
        float shield = 0.f;
        Ledge* ledge = nullptr;
    };

    //--------------------------------------------------------//

    virtual ~Fighter();

    virtual void tick() = 0;

    //--------------------------------------------------------//

    const uint8_t index;

    const FighterEnum type;

    FightWorld& world;

    Stats stats;

    Status status;

    //--------------------------------------------------------//

    struct InterpolationData
    {
        Vec3F translation;
        QuatF rotation;
        sq::Armature::Pose pose;
    }
    previous, current;

    //--------------------------------------------------------//

    const sq::Armature& get_armature() const { return mArmature; }

    void set_controller(Controller* controller) { mController = controller; }
    Controller* get_controller() { return mController; };

    Action* get_action(ActionType type);

    //--------------------------------------------------------//

    /// Called when hit by a basic attack.
    void apply_hit_basic(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when passing the stage boundary.
    void pass_boundary();

    //-- private member access methods -----------------------//

    /// Get current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get current armature pose matrices (local, transposed).
    const std::vector<Mat34F>& get_bone_matrices() const { return mBoneMatrices; }

    /// Get the collision diamond.
    const LocalDiamond& get_diamond() const { return mLocalDiamond; }

    //-- compute data needed for update and render -----------//

    /// Compute matrix for the specified bone, or model matrix if -1.
    Mat4F get_bone_matrix(int8_t bone) const;

    /// Compute interpolated model matrix.
    Mat4F interpolate_model_matrix(float blend) const;

    /// Compute interpolated armature pose matrices.
    void interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const;

    //-- lua properties and methods --------------------------//

    bool lua_get_intangible() const { return status.intangible; }
    void lua_set_intangible(bool value) { status.intangible = value; }

    bool lua_get_autocancel() const { return status.autocancel; }
    void lua_set_autocancel(bool value) { status.autocancel = value; }

    float lua_get_velocity_x() const { return status.velocity.x; }
    void lua_set_velocity_x(float value) { status.velocity.x = value; }

    float lua_get_velocity_y() const { return status.velocity.y; }
    void lua_set_velocity_y(float value) { status.velocity.y = value; }

    int lua_get_facing() const { return int(status.facing); }

protected: //=================================================//

    // todo: I'd like not even have c++ classes for specific fighters,
    // it'd be great to be able to do all the customisation in lua.
    // Until then some stuff is protected instead of private.

    Fighter(uint8_t index, FightWorld& world, FighterEnum type);

    void base_tick_fighter();

    uint mStateProgress = 0u;

private: //===================================================//

    //-- init methods, called by constructor -----------------//

    void initialise_armature(const String& path);

    void initialise_hurt_blobs(const String& path);

    void initialise_stats(const String& path);

    void initialise_actions(const String& path);

    //-- control and command buffer stuff --------------------//

    bool consume_command(Command cmd);

    bool consume_command_oldest(std::initializer_list<Command> cmds);

    bool consume_command_facing(Command leftCmd, Command rightCmd);

    bool consume_command_oldest_facing(std::initializer_list<Command> leftCmds, std::initializer_list<Command> rightCmds);

    Controller* mController = nullptr;

    std::array<std::vector<Command>, CMD_BUFFER_SIZE> mCommands;

    //-- methods used internally and by the action editor ----//

    /// Transition from one state to another and play animations.
    void state_transition(State state, uint fadeNow, const Animation* animNow, uint fadeAfter, const Animation* animAfter);

    /// Activate an action and change to the appropriate state.
    void state_transition_action(ActionType action);

    /// If we have an active and started action, cancel and deactivate it.
    void cancel_active_action();

    //-- update methods, called each tick --------------------//

    void update_commands(const InputFrame& input);

    void update_action(const InputFrame& input);

    void update_transitions(const InputFrame& input);

    void update_states(const InputFrame& input);

    void update_animation();

    //--------------------------------------------------------//

    sq::Armature mArmature;

    Animations mAnimations;

    std::array<std::unique_ptr<Action>, sq::enum_count_v<ActionType>> mActions;

    sq::PoolMap<TinyString, HurtBlob> mHurtBlobs;

    LocalDiamond mLocalDiamond;

    //--------------------------------------------------------//

    Action* mActiveAction = nullptr;

    //uint mStateProgress = 0u;

    uint mLandingTime = 0u;

    bool mJumpHeld = false;

    uint mTimeSinceLedge = 0u;
    uint mExtraJumps = 0u;

    uint mFreezeTime = 0u;
    uint mFrozenProgress = 0u;
    State mFrozenState = {};

    float mLaunchSpeed = 0.f;
    uint mHitStunTime = 0u;

    bool mVertigoActive = false;

    Vec2F mTranslate = { 0.f, 0.f };

    //--------------------------------------------------------//

    const Animation* mAnimation = nullptr;

    uint mNextFadeFrames = 0u;
    const Animation* mNextAnimation = nullptr;

    uint mAnimTimeDiscrete = 0u;
    float mAnimTimeContinuous = 0.f;

    //bool mAnimChangeFacing = false;

    Vec3F mPrevRootMotionOffset = Vec3F();

    QuatF mFadeStartRotation;
    sq::Armature::Pose mFadeStartPose;

    uint mFadeProgress = 0u;
    uint mFadeFrames = 0u;

    // these are only used for debug
    const Animation* mPreviousAnimation = nullptr;
    uint mPreviousAnimTimeDiscrete = 0u;
    float mPreviousAnimTimeContinuous = 0.f;

    //--------------------------------------------------------//

    std::vector<Mat34F> mBoneMatrices;

    Mat4F mModelMatrix;

public: //== debug and editor stuff ==========================//

    void debug_reload_actions();

    std::vector<Mat4F> debug_get_skeleton_mats() const;

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts
