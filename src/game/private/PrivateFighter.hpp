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
    using Facing = Fighter::Facing;
    using Stats = Fighter::Stats;
    using Status = Fighter::Status;
    using AnimMode = Fighter::AnimMode;

    using Animation = Fighter::Animation;
    using Transition = Fighter::Transition;

    //--------------------------------------------------------//

    struct Transitions
    {
        Transition neutral_crouch;
        Transition neutral_shield;
        Transition neutral_walking;

        Transition walking_crouch;
        Transition walking_shield;
        Transition walking_dashing;
        Transition walking_dive;
        Transition walking_neutral;

        Transition dashing_shield;
        Transition dashing_brake;
        Transition dashing_dive;

        Transition neutral_turn;

        Transition brake_turn_brake;
        Transition brake_turn_dash;

        Transition crouch_shield;
        Transition crouch_neutral;

        Transition air_dodge;
        Transition air_hop;
        Transition air_ledge;

        Transition jumping_falling;

        Transition shield_dodge;
        Transition shield_evade_back;
        Transition shield_evade_forward;
        Transition shield_neutral;

        Transition ledge_climb;
        Transition ledge_jump;
        Transition ledge_drop;

        Transition misc_vertigo;
        Transition misc_prejump;

        Transition jump_back;
        Transition jump_forward;
        Transition land_clean;

        Transition instant_crouch;
        Transition instant_falling;
        Transition instant_neutral;
        Transition instant_shield;
        Transition instant_dashing;

        Transition neutral_attack;

        Transition tilt_down_attack;
        Transition tilt_forward_attack;
        Transition tilt_up_attack;

        Transition air_back_attack;
        Transition air_down_attack;
        Transition air_forward_attack;
        Transition air_neutral_attack;
        Transition air_up_attack;

        Transition dash_attack;

        Transition smash_down_start;
        Transition smash_forward_start;
        Transition smash_up_start;

        Transition smash_down_attack;
        Transition smash_forward_attack;
        Transition smash_up_attack;

        Transition editor_preview;
    };

    //--------------------------------------------------------//

    struct InterpolationData
    {
        Vec2F position { 0.f, 1.f };
        sq::Armature::Pose pose;
    }
    previous, current;

    //--------------------------------------------------------//

    Controller* controller = nullptr;

    Transitions transitions;

    sq::Armature armature;

    //-- init functions, called by constructor ---------------//

    void initialise_armature(const String& path);

    void initialise_hurt_blobs(const String& path);

    void initialise_stats(const String& path);

    void initialise_actions(const String& path);

    //-- called by passthrough methods in Fighter ------------//

    void base_tick_fighter();

    void base_tick_animation();

    //-- used internally and by the action editor ------------//

    void state_transition(const Transition& transition, bool keepTime = false);

    void switch_action(ActionType type);

private: //===================================================//

    int8_t mMoveAxisX = 0;
    int8_t mMoveAxisY = 0;

    int8_t mDoBrakeTurn = 0;

    bool mJumpHeld = false;
    bool mDoTurnDash = false;

    float mJumpVelocity = 0.f;
    float mBrakeVelocity = 0.f;

    uint mStateProgress = 0u;

    //--------------------------------------------------------//

    const Animation* mAnimation = nullptr;
    const Animation* mNextAnimation = nullptr;
    const sq::Armature::Pose* mStaticPose = nullptr;

    int mAnimTimeDiscrete = 0;
    float mAnimTimeContinuous = 0.f;
    float mAnimScaleTime = 1.f;

    bool mVertigoActive = false;

    Vec2F mPrevRootMotionOffset = Vec2F();

    sq::Armature::Pose mFadeStartPose;

    uint mFadeFrames = 0u;
    uint mFadeProgress = 0u;

    //--------------------------------------------------------//

    void handle_input_movement(const Controller::Input& input);

    void handle_input_actions(const Controller::Input& input);

    void update_after_input();

    //--------------------------------------------------------//

    FightWorld& get_world() { return fighter.mFightWorld; }

    //--------------------------------------------------------//

    Fighter& fighter;
};

//============================================================================//

} // namespace sts
