#pragma once

#include "setup.hpp"

#include "game/ActionEnums.hpp"
#include "game/FighterEnums.hpp"
#include "main/MainEnums.hpp"

#include <sqee/app/WrenPlus.hpp>
#include <sqee/objects/Armature.hpp>

namespace sts {

//============================================================================//

struct InputFrame;
struct Ledge;

//============================================================================//

struct LocalDiamond
{
    void compute_normals()
    {
        normLeftDown = sq::maths::normalize(Vec2F(-offsetCross, -halfWidth));
        normLeftUp = sq::maths::normalize(Vec2F(-offsetCross, +halfWidth));
        normRightDown = sq::maths::normalize(Vec2F(+offsetCross, -halfWidth));
        normRightUp = sq::maths::normalize(Vec2F(+offsetCross, +halfWidth));
    }

    Vec2F min() const { return { -halfWidth, 0.f }; }
    Vec2F max() const { return { +halfWidth, offsetTop }; }
    Vec2F cross() const { return { 0.f, offsetCross }; }

    float halfWidth, offsetCross, offsetTop;
    Vec2F normLeftDown, normLeftUp, normRightDown, normRightUp;
};

//============================================================================//

class Fighter final : sq::NonCopyable
{
public: //====================================================//

    using State = FighterState;
    using Command = FighterCommand;
    using AnimMode = FighterAnimMode;

    //--------------------------------------------------------//

    /// One animation with related metadata.
    struct Animation
    {
        sq::Armature::Animation anim;
        AnimMode mode;
        String key;

        bool is_looping() const
        {
            return mode == AnimMode::Looping || mode == AnimMode::WalkCycle || mode == AnimMode::DashCycle;
        }
    };

    //--------------------------------------------------------//

    /// Structure for the basic animations used by a fighter.
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
        Animation NeutralSecond;
        Animation NeutralThird;

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

        Animation HurtMiddleTumble;
        Animation HurtLowerTumble;
        Animation HurtUpperTumble;

        Animation HurtMiddleHeavy;
        Animation HurtLowerHeavy;
        Animation HurtUpperHeavy;

        Animation HurtMiddleLight;
        Animation HurtLowerLight;
        Animation HurtUpperLight;

        Animation HurtAirLight;
        Animation HurtAirHeavy;

        //Animation LaunchLoop;
        //Animation LaunchFinish;
    };

    //--------------------------------------------------------//

    /// Structure for stats that generally don't change during a game.
    struct Attributes
    {
        float walk_speed        = 0.1f;
        float dash_speed        = 0.15f;
        float air_speed         = 0.1f;
        float traction          = 0.005f;
        float air_mobility      = 0.008f;
        float air_friction      = 0.002f;
        float hop_height        = 1.5f;
        float jump_height       = 3.5f;
        float airhop_height     = 3.5f;
        float gravity           = 0.01f;
        float fall_speed        = 0.15f;
        float fastfall_speed    = 0.24f;
        float weight            = 100.f;

        uint extra_jumps = 2u;

        uint land_heavy_fall_time = 4u;

        uint dash_start_time  = 11u;
        uint dash_brake_time  = 12u;
        uint dash_turn_time   = 14u;

        float anim_walk_stride = 2.f;
        float anim_dash_stride = 3.f;
    };

    //--------------------------------------------------------//

    /// Structure for the public state and status of a fighter.
    struct Status
    {
        State state = State::Neutral;
        int8_t facing = +1;
        Vec2F position = { 0.f, 0.f };
        Vec2F velocity = { 0.f, 0.f };
        bool intangible = false;
        bool autocancel = false;
        float damage = 0.f;
        float shield = SHIELD_MAX_HP;
        Ledge* ledge = nullptr;
    };

    //--------------------------------------------------------//

    /// Structure for data to interpolate between ticks.
    struct InterpolationData
    {
        Vec3F translation;
        QuatF rotation;
        sq::Armature::Pose pose;
    };

    //--------------------------------------------------------//

    Fighter(FightWorld& world, FighterEnum type, uint8_t index);

    ~Fighter();

    void tick();

    //--------------------------------------------------------//

    FightWorld& world;

    const FighterEnum type;

    const uint8_t index;

    Attributes attributes;

    Status status;

    InterpolationData previous, current;

    //--------------------------------------------------------//

    const sq::Armature& get_armature() const { return mArmature; }

    void set_controller(Controller* controller) { mController = controller; }
    Controller* get_controller() { return mController; };

    Action* get_action(ActionType type);

    //--------------------------------------------------------//

    /// Call the appropriate function based on current state.
    void apply_hit_generic(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when hit while on the ground.
    //void apply_hit_ground(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when hit while in the air.
    //void apply_hit_air(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when hit while shield is active.
    //void apply_hit_shield(const HitBlob& hit, const HurtBlob& hurt);

    /// Called when shield is broken by attack or decay.
    void apply_shield_break();

    /// Called when passing the stage boundary.
    void pass_boundary();

    //-- private member access methods -----------------------//

    /// Get current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get current armature pose matrices (local, transposed).
    const std::vector<Mat34F>& get_bone_matrices() const { return mBoneMatrices; }

    /// Get the collision diamond.
    const LocalDiamond& get_diamond() const { return mLocalDiamond; }

    /// Get the active action.
    const Action* get_active_action() const { return mActiveAction; }

    //-- compute data needed for update and render -----------//

    /// Compute matrix for the specified bone, or model matrix if -1.
    Mat4F get_bone_matrix(int8_t bone) const;

    /// Should hurt/flinch models be used if available.
    bool should_render_flinch_models() const;

    //-- wren methods and properties -------------------------//

    void wren_reset_collisions();

    void wren_set_intangible(bool value);

    void wren_enable_hurtblob(TinyString key);

    void wren_disable_hurtblob(TinyString key);

    void wren_set_velocity_x(float value);

    void wren_set_autocancel(bool value);

private: //===================================================//

    //-- init methods, called by constructor -----------------//

    void initialise_attributes(const String& path);

    void initialise_armature(const String& path);

    void initialise_hurtblobs(const String& path);

    //-- control and command buffer stuff --------------------//

    bool consume_command(Command cmd);

    bool consume_command(std::initializer_list<Command> cmds);

    bool consume_command_facing(Command leftCmd, Command rightCmd);

    bool consume_command_facing(std::initializer_list<Command> leftCmds, std::initializer_list<Command> rightCmds);

    Controller* mController = nullptr;

    std::array<std::vector<Command>, CMD_BUFFER_SIZE> mCommands;

    //-- methods used internally and by the action editor ----//

    /// Transition from one state to another and play animations.
    void state_transition(State state, uint fadeNow, const Animation* animNow, uint fadeAfter, const Animation* animAfter);

    /// Activate an action and change to the appropriate state.
    void state_transition_action(ActionType action);

    /// Finish a charge action and switch to its attack action.
    void state_transition_charge_done();

    /// If we have an active and started action, cancel and deactivate it.
    void cancel_active_action();

    //-- update methods, called each tick --------------------//

    void update_commands(const InputFrame& input);

    void update_transitions(const InputFrame& input);

    void update_states(const InputFrame& input);

    void update_action(const InputFrame& input);

    void update_animation();

    //--------------------------------------------------------//

    sq::Armature mArmature;

    Animations mAnimations;

    std::array<std::unique_ptr<Action>, sq::enum_count_v<ActionType>> mActions;

    std::pmr::map<TinyString, HurtBlob> mHurtBlobs;

    LocalDiamond mLocalDiamond;

    //--------------------------------------------------------//

    // todo: comments for this stuff would be nice

    Action* mActiveAction = nullptr;

    ActionType mForceSwitchAction = ActionType::None;

    uint mStateProgress = 0u;

    uint mLandingTime = 0u;

    bool mJumpHeld = false;

    uint mTimeSinceLedge = 0u;
    uint mTimeSinceFallSpeed = 0u;
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

    Vec3F mPrevRootMotionOffset = Vec3F();

    QuatF mFadeStartRotation;
    sq::Armature::Pose mFadeStartPose;

    uint mFadeProgress = 0u;
    uint mFadeFrames = 0u;

    std::vector<Mat34F> mBoneMatrices;

    Mat4F mModelMatrix;

    //-- debug and editor stuff ------------------------------//

    const Animation* mPreviousAnimation = nullptr;
    uint mPreviousAnimTimeDiscrete = 0u;
    float mPreviousAnimTimeContinuous = 0.f;

    friend DebugGui;
    friend EditorScene;
};

//============================================================================//

} // namespace sts

template<> struct wren::Traits<sts::Fighter> : std::true_type
{
    static constexpr const char module[] = "sts";
    static constexpr const char className[] = "Fighter";
};
