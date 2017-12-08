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

    //-- called by passthrough methods in Fighter ------------//

    void base_tick_fighter();

    void base_tick_animation();

private: //===================================================//

    bool mAttackHeld = false;
    bool mJumpHeld = false;

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

    //-- should be the only places input is handled ----------//

    void handle_input_movement(const Controller::Input& input);

    void handle_input_actions(const Controller::Input& input);

    //--------------------------------------------------------//

    void state_transition(const Transition& transition);

    void switch_action(Action::Type actionType);

    //--------------------------------------------------------//

    void update_physics();

    void update_active_action();

    //--------------------------------------------------------//

    Fighter& fighter;
};

//============================================================================//

} // namespace sts
