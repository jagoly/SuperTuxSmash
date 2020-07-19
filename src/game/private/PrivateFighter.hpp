#pragma once

#include <sqee/render/Armature.hpp>

#include "game/Controller.hpp"
#include "game/Fighter.hpp"

namespace sts {

//============================================================================//

class PrivateFighter final
{
public: //====================================================//

    PrivateFighter(Fighter& fighter) : fighter(fighter) {}

    //--------------------------------------------------------//

    using State = Fighter::State;
    using AnimMode = Fighter::AnimMode;
    using Animation = Fighter::Animation;

    //--------------------------------------------------------//

    struct Animations
    {
        Animation* begin() { return static_cast<Animation*>(static_cast<void*>(this)); }
        Animation* end() { return begin() + sizeof(Animations) / sizeof(Animation); }

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

    //--------------------------------------------------------//

    struct InterpolationData
    {
        Vec2F position { 0.f, 1.f };
        QuatF rotation;
        sq::Armature::Pose pose;
    }
    previous, current;

    //--------------------------------------------------------//

    Controller* controller = nullptr;

    Animations animations;

    sq::Armature armature;

    //-- init functions, called by constructor ---------------//

    void initialise_armature(const String& path);

    void initialise_hurt_blobs(const String& path);

    void initialise_stats(const String& path);

    void initialise_actions(const String& path);

    //-- called by passthrough methods in Fighter ------------//

    void base_tick_fighter();

    //-- used internally and by the action editor ------------//

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

private: //===================================================//

    bool mJumpHeld = false;

    uint mStateProgress = 0u;

    uint mExtraJumps = 0u;

    uint mLandingLag = 0u;

    uint mTimeSinceLedge = 0u;

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

    bool mVertigoActive = false;

    // these are only used for debug
    const Animation* mPreviousAnimation = nullptr;
    uint mPreviousAnimTimeDiscrete = 0u;
    float mPreviousAnimTimeContinuous = 0.f;

    //--------------------------------------------------------//

    void update_commands(const Controller::Input& input);

    void update_transitions(const Controller::Input& input);

    void update_states(const Controller::Input& input);

    void update_animation();

    //--------------------------------------------------------//

    FightWorld& get_world() { return fighter.mFightWorld; }

    //--------------------------------------------------------//

    Fighter& fighter;

    friend class EditorScene;
    friend struct DebugGui;
};

//============================================================================//

} // namespace sts
