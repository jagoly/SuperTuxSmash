#pragma once

#include <sqee/misc/Builtins.hpp>

#include <sqee/render/Armature.hpp>

#include "game/Actions.hpp"
#include "game/Controller.hpp"
#include "game/FightWorld.hpp"

namespace sts {

//====== Constants and Forward Declarations ==================================//

inline constexpr uint STS_CMD_BUFFER_SIZE = 8u;
inline constexpr uint STS_LIGHT_LANDING_LAG = 2u;
inline constexpr uint STS_HEAVY_LANDING_LAG = 8u;
inline constexpr uint STS_JUMP_DELAY = 5u;
inline constexpr uint STS_NO_LEDGE_CATCH_TIME = 48u;
inline constexpr uint STS_MIN_LEDGE_HANG_TIME = 16u;

struct Ledge; // status.ledge

//============================================================================//

class Fighter : sq::NonCopyable
{
public: //====================================================//

    enum class State
    {
        Neutral,
        Walking,
        Dashing,
        Brake,
        Crouch,
        Charge,
        Action,
        AirAction,
        Landing,
        PreJump,
        Jumping,
        Helpless,
        Knocked,
        Stunned,
        Shield,
        LedgeHang,
        LedgeClimb,
        EditorPreview,
    };

    enum class Command
    {
        Shield,        ///< used to perform evades/dodges
        Jump,          ///< used to jump and airhop
        MashDown,      ///< used to drop through platforms and fastfall
        MashUp,        ///< not currently used
        MashLeft,      ///< used to start dashing
        MashRight,     ///< used to start dashing
        TurnLeft,      ///< used to change facing
        TurnRight,     ///< used to change facing
        SmashDown,     ///< begin charging a down smash
        SmashUp,       ///< begin charging an up smash
        SmashLeft,     ///< begin charging a forward smash
        SmashRight,    ///< begin charging a forward smash
        AttackDown,    ///< perform dtilt, dair or dash attack
        AttackUp,      ///< perform utilt, uair or dash attack
        AttackLeft,    ///< perform ftilt, fair, bair or dash attack
        AttackRight,   ///< perform ftilt, fair, bair or dash attack
        AttackNeutral, ///< perform neutral, nair or dash attack
    };

    enum class AnimMode
    {
        Standard,    ///< non-looping, without root motion
        Looping,     ///< looping, without root motion
        WalkCycle,   ///< looping, update using velocity and anim_walk_stride
        DashCycle,   ///< looping, update using velocity and anim_dash_stride
        ApplyMotion, ///< non-looping, extract root motion to object
    };

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
        Animation LandLight;
        Animation LandHeavy;
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

        Animation EvadeBack;
        Animation EvadeForward;
        Animation Dodge;
        Animation AirDodge;

        Animation NeutralFirst;

        Animation TiltDown;
        Animation TiltForward;
        Animation TiltUp;

        Animation AirBack;
        Animation AirDown;
        Animation AirForward;
        Animation AirNeutral;
        Animation AirUp;

        Animation DashAttack;

        Animation SmashDownStart;
        Animation SmashForwardStart;
        Animation SmashUpStart;

        Animation SmashDownCharge;
        Animation SmashForwardCharge;
        Animation SmashUpCharge;

        Animation SmashDownAttack;
        Animation SmashForwardAttack;
        Animation SmashUpAttack;
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
        float weight        = 1.f;

        uint extra_jumps = 2u;

        uint land_heavy_min_time = 4u;

        uint dash_start_time  = 11u;
        uint dash_brake_time  = 12u;
        uint dash_turn_time   = 14u;
        uint ledge_climb_time = 32u;

        float anim_walk_stride = 2.f;
        float anim_dash_stride = 3.f;
    };

    /// Structure containing the public state and status of a fighter.
    struct Status
    {
        State state = State::Neutral;
        int8_t facing = +1;
        Vec2F velocity = { 0.f, 0.f };
        bool intangible = false;
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
        Vec2F position { 0.f, 0.f };
        QuatF rotation { 0.f, 0.f, 0.f, 1.f };
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
    void apply_hit_basic(const HitBlob& hit);

    /// Called when passing the stage boundary.
    void pass_boundary();

    //-- access data needed for world updates ----------------//

    /// Get current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get current armature pose matrices.
    const Vector<Mat34F>& get_bone_matrices() const { return mBoneMatrices; }

    /// Get current position (same as a model matrix)
    Vec2F get_position() const { return current.position; }

    /// Get current velocity.
    Vec2F get_velocity() const { return status.velocity; }

    /// Get the collision diamond.
    const LocalDiamond& get_diamond() const { return mLocalDiamond; }

    //-- compute data needed for rendering -------------------//

    /// Compute interpolated model matrix.
    Mat4F interpolate_model_matrix(float blend) const;

    /// Compute interpolated armature pose matrices.
    void interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const;

    //-- lua properties and methods --------------------------//

    bool lua_get_intangible() const { return status.intangible; }
    void lua_set_intangible(bool value) { status.intangible = value; }

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

private: //===================================================//

    //-- init methods, called by constructor -----------------//

    void initialise_armature(const String& path);

    void initialise_hurt_blobs(const String& path);

    void initialise_stats(const String& path);

    void initialise_actions(const String& path);

    //-- control and command buffer stuff --------------------//

    bool consume_command(Command cmd);

    bool consume_command_oldest(InitList<Command> cmds);

    bool consume_command_facing(Command leftCmd, Command rightCmd);

    bool consume_command_oldest_facing(InitList<Command> leftCmds, InitList<Command> rightCmds);

    Controller* mController = nullptr;

    Array<Vector<Command>, STS_CMD_BUFFER_SIZE> mCommands;

    //-- methods used internally and by the action editor ----//

    /// Transition from one state to another and play animations
    ///
    /// @param newState     the state to change to
    /// @param fadeNow      fade into animNow over this many frames
    /// @param animNow      play this animation immediately
    /// @param fadeAfter    fade into animAfter over this many frames
    /// @param animAfter    play after the current anim finishes

    void state_transition(State newState, uint fadeNow, const Animation* animNow,
                          uint fadeAfter, const Animation* animAfter);

    void switch_action(ActionType type);

    //-- update methods, called each tick --------------------//

    void update_commands(const Controller::Input& input);

    void update_transitions(const Controller::Input& input);

    void update_states(const Controller::Input& input);

    void update_animation();

    //--------------------------------------------------------//

    sq::Armature mArmature;

    Animations mAnimations;

    Array<UniquePtr<Action>, sq::enum_count_v<ActionType>> mActions;

    sq::PoolMap<TinyString, HurtBlob> mHurtBlobs;

    LocalDiamond mLocalDiamond;

    //--------------------------------------------------------//

    Action* mActiveAction = nullptr;

    uint mStateProgress = 0u;

    uint mExtraJumps = 0u;

    uint mLandingLag = 0u;

    uint mTimeSinceLedge = 0u;

    bool mJumpHeld = false;

    bool mVertigoActive = false;

    Vec2F mTranslate = { 0.f, 0.f };

    //--------------------------------------------------------//

    const Animation* mAnimation = nullptr;

    uint mNextFadeFrames = 0u;
    const Animation* mNextAnimation = nullptr;

    uint mAnimTimeDiscrete = 0u;
    float mAnimTimeContinuous = 0.f;

    bool mAnimChangeFacing = false;

    Vec3F mPrevRootMotionOffset = Vec3F();

    sq::Armature::Pose mFadeStartPose;
    QuatF mFadeStartRotation;

    uint mFadeProgress = 0u;
    uint mFadeFrames = 0u;

    // these are only used for debug
    const Animation* mPreviousAnimation = nullptr;
    uint mPreviousAnimTimeDiscrete = 0u;
    float mPreviousAnimTimeContinuous = 0.f;

    //--------------------------------------------------------//

    Vector<Mat34F> mBoneMatrices;

    Mat4F mModelMatrix;

public: //== debug and editor stuff ==========================//

    void debug_reload_actions();

    Vector<Mat4F> debug_get_skeleton_mats() const;

    friend class EditorScene;
    friend struct DebugGui;
};

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::Fighter::State,
                 Neutral,
                 Walking,
                 Dashing,
                 Brake,
                 Crouch,
                 Charge,
                 Action,
                 AirAction,
                 Landing,
                 PreJump,
                 Jumping,
                 Helpless,
                 Knocked,
                 Stunned,
                 Shield,
                 LedgeHang,
                 LedgeClimb,
                 EditorPreview)

SQEE_ENUM_HELPER(sts::Fighter::Command,
                 Shield,
                 Jump,
                 MashDown,
                 MashUp,
                 MashLeft,
                 MashRight,
                 TurnLeft,
                 TurnRight,
                 SmashDown,
                 SmashUp,
                 SmashLeft,
                 SmashRight,
                 AttackDown,
                 AttackUp,
                 AttackLeft,
                 AttackRight,
                 AttackNeutral)

SQEE_ENUM_HELPER(sts::Fighter::AnimMode,
                 Standard,
                 Looping,
                 WalkCycle,
                 DashCycle,
                 ApplyMotion)
