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

    //--------------------------------------------------------//

    using Animation = sq::Armature::Animation;

    struct Transition
    {
        State newState; uint fadeFrames;
        const Animation* animation;
        const Animation* loop = nullptr;
    };

    //--------------------------------------------------------//

    struct Animations
    {
        Animation crouch_loop;
        Animation dashing_loop;
        Animation falling_loop;
        Animation jumping_loop;
        Animation neutral_loop;
        Animation shield_loop;
        Animation walking_loop;

        Animation airdodge; // dodge in the air
        Animation airhop;   // begin an extra jump
        Animation brake;    // finish dashing
        Animation crouch;   // begin crouching
        Animation divedash; // dash off an edge
        Animation divewalk; // walk off an edge
        Animation dodge;    // dodge on the ground
        Animation evade;    // evade back or forward
        Animation jump;     // begin a normal jump
        Animation land;     // land on the ground
        Animation stand;    // finish crouching
        Animation unshield; // finish shielding

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
        Transition neutral_walking;

        Transition walking_crouch;
        Transition walking_dashing;
        Transition walking_dive;
        Transition walking_neutral;

        Transition dashing_brake;
        Transition dashing_dive;

        Transition crouch_stand;

        Transition jumping_dodge;
        Transition jumping_falling;
        Transition jumping_hop;

        Transition shield_dodge;
        Transition shield_evade;
        Transition shield_neutral;

        Transition falling_dodge;
        Transition falling_hop;

        Transition misc_jump;
        Transition misc_land;

        Transition misc_crouch;
        Transition misc_falling;
        Transition misc_neutral;
        Transition misc_shield;

        Transition smash_down_start;
        Transition smash_forward_start;
        Transition smash_up_start;

        Transition smash_down_attack;
        Transition smash_forward_attack;
        Transition smash_up_attack;
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

    Animations animations;
    Transitions transitions;

    sq::Armature armature;

    //-- init functions, called by constructor ---------------//

    void initialise_armature(const string& path);

    void initialise_hurt_blobs(const string& path);

    void initialise_stats(const string& path);

    void initialise_actions(const string& path);

    //-- called by passthrough methods in Fighter ------------//

    void base_tick_fighter();

    void base_tick_animation();

private: //===================================================//

    bool mAttackHeld = false;
    bool mJumpHeld = false;

    uint mStateProgress = 0u;

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

    void handle_input_movement(const Controller::Input& input);

    void handle_input_actions(const Controller::Input& input);

    void update_after_input();

    //--------------------------------------------------------//

    void state_transition(const Transition& transition);

    void switch_action(Action::Type actionType);

    //--------------------------------------------------------//

    Fighter& fighter;
};

//============================================================================//

} // namespace sts
