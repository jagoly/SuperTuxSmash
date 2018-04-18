#include <sqee/assert.hpp>
#include <sqee/debug/Logging.hpp>

#include <sqee/macros.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

#include <sqee/maths/Functions.hpp>

#include "game/Stage.hpp"

#include "game/private/PrivateFighter.hpp"

namespace maths = sq::maths;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

static constexpr float STS_WALK_SPEED       = 3.0f;  // walk_speed
static constexpr float STS_DASH_SPEED       = 6.0f;  // dash_speed
static constexpr float STS_AIR_SPEED        = 3.0f;  // air_speed

static constexpr float STS_WALK_MOBILITY    = 1.0f;  // traction
static constexpr float STS_DASH_MOBILITY    = 1.5f;  // traction
static constexpr float STS_LAND_FRICTION    = 0.5f;  // traction

static constexpr float STS_AIR_MOBILITY     = 0.5f;  // air_mobility
static constexpr float STS_AIR_FRICTION     = 0.2f;  // air_friction

static constexpr float STS_HOP_HEIGHT       = 2.0f;  // hop_height
static constexpr float STS_JUMP_HEIGHT      = 3.0f;  // jump_height
static constexpr float STS_AIR_HOP_HEIGHT   = 2.0f;  // air_hop_height

static constexpr float STS_GRAVITY          = 0.5f;  // gravity
static constexpr float STS_FALL_SPEED       = 12.0f; // fall_speed

static constexpr float STS_EVADE_DISTANCE   = 2.0f;  // evade_distance

static constexpr float STS_TICK_RATE        = 48.0f;
static constexpr uint  STS_LANDING_LAG      = 4u;
static constexpr uint  STS_JUMP_DELAY       = 4u;

//============================================================================//

void PrivateFighter::initialise_armature(const string& path)
{
    armature.load_bones(path + "/Bones.txt", true);
    armature.load_rest_pose(path + "/RestPose.txt");

    current.pose = previous.pose = armature.get_rest_pose();

    //--------------------------------------------------------//

    const auto load_animation = [&](sq::Armature::Animation& anim, const char* name)
    {
        const string filePath = sq::build_path(path, "anims", name) + ".txt";

        if (sq::check_file_exists(filePath) == false)
        {
            sq::log_warning("missing animation '%s'", filePath);
            anim = armature.make_animation(path + "/anims/Null.txt");
            return;
        }

        anim = armature.make_animation(filePath);
    };

    //--------------------------------------------------------//

    Animations& a = animations;
    Transitions& t = transitions;

    //--------------------------------------------------------//

    load_animation ( a.crouch_loop,  "CrouchLoop"  );
    load_animation ( a.dashing_loop, "DashingLoop" );
    load_animation ( a.falling_loop, "FallingLoop" );
    load_animation ( a.jumping_loop, "JumpingLoop" );
    load_animation ( a.neutral_loop, "NeutralLoop" );
    load_animation ( a.shield_loop,  "ShieldLoop"  );
    load_animation ( a.vertigo_loop, "VertigoLoop" );
    load_animation ( a.walking_loop, "WalkingLoop" );

    load_animation ( a.airdodge, "Airdodge" );
    load_animation ( a.airhop,   "Airhop"   );
    load_animation ( a.brake,    "Brake"    );
    load_animation ( a.crouch,   "Crouch"   );
    load_animation ( a.divedash, "Divedash" );
    load_animation ( a.divewalk, "Divewalk" );
    load_animation ( a.dodge,    "Dodge"    );
    load_animation ( a.evade,    "Evade"    );
    load_animation ( a.jump,     "Jump"     );
    load_animation ( a.land,     "Land"     );
    load_animation ( a.stand,    "Stand"    );
    load_animation ( a.unshield, "Unshield" );

    load_animation ( a.action_neutral_first, "actions/NeutralFirst" );

    load_animation ( a.action_tilt_down,    "actions/TiltDown"    );
    load_animation ( a.action_tilt_forward, "actions/TiltForward" );
    load_animation ( a.action_tilt_up,      "actions/TiltUp"      );

    load_animation ( a.action_air_back,    "actions/AirBack"    );
    load_animation ( a.action_air_down,    "actions/AirDown"    );
    load_animation ( a.action_air_forward, "actions/AirForward" );
    load_animation ( a.action_air_neutral, "actions/AirNeutral" );
    load_animation ( a.action_air_up,      "actions/AirUp"      );

    load_animation ( a.action_dash_attack, "actions/DashAttack" );

    load_animation ( a.action_smash_down_start,    "action/SmashDownStart"    );
    load_animation ( a.action_smash_forward_start, "action/SmashForwardStart" );
    load_animation ( a.action_smash_up_start,      "action/SmashUpStart"      );

    load_animation ( a.action_smash_down_charge,    "action/SmashDownCharge"    );
    load_animation ( a.action_smash_forward_charge, "action/SmashForwardCharge" );
    load_animation ( a.action_smash_up_charge,      "action/SmashUpCharge"      );

    load_animation ( a.action_smash_down_attack,    "action/SmashDownAttack"    );
    load_animation ( a.action_smash_forward_attack, "action/SmashForwardAttack" );
    load_animation ( a.action_smash_up_attack,      "action/SmashUpAttack"      );

    //--------------------------------------------------------//

    t.neutral_crouch  = { State::Crouch,  2u, &a.crouch, &a.crouch_loop };
    t.neutral_walking = { State::Walking, 4u, &a.walking_loop, nullptr };

    t.walking_crouch  = { State::Crouch,  2u, &a.crouch, &a.crouch_loop };
    t.walking_dashing = { State::Dashing, 4u, &a.dashing_loop, nullptr };
    t.walking_dive    = { State::Falling, 2u, &a.divewalk, &a.falling_loop };
    t.walking_neutral = { State::Neutral, 4u, &a.neutral_loop, nullptr };

    t.dashing_brake = { State::Brake,   2u, &a.brake, &a.neutral_loop };
    t.dashing_dive  = { State::Falling, 2u, &a.divedash, &a.falling_loop };

    t.crouch_stand = { State::Neutral, 2u, &a.stand, &a.neutral_loop };

    t.jumping_dodge   = { State::AirDodge, 1u, &a.airdodge, &a.falling_loop };
    t.jumping_falling = { State::Falling,  8u, &a.falling_loop, nullptr };
    t.jumping_hop     = { State::Jumping,  1u, &a.airhop, &a.jumping_loop };

    t.shield_dodge   = { State::Dodge,   1u, &a.dodge, &a.neutral_loop };
    t.shield_evade   = { State::Evade,   1u, &a.evade, &a.neutral_loop };
    t.shield_neutral = { State::Neutral, 1u, &a.unshield, &a.neutral_loop };

    t.falling_dodge = { State::AirDodge, 1u, &a.airdodge, &a.falling_loop };
    t.falling_hop   = { State::Jumping,  1u, &a.airhop, &a.jumping_loop };

    t.misc_jump = { State::PreJump, 1u, &a.jump, &a.jumping_loop };
    t.misc_land = { State::Landing, 1u, &a.land, &a.neutral_loop };

    t.misc_crouch  = { State::Crouch,  2u, &a.crouch_loop, nullptr };
    t.misc_falling = { State::Falling, 2u, &a.falling_loop, nullptr };
    t.misc_neutral = { State::Neutral, 2u, &a.neutral_loop, nullptr };
    t.misc_shield  = { State::Shield,  2u, &a.shield_loop, nullptr };
    t.misc_vertigo = { State::Neutral, 2u, &a.vertigo_loop, nullptr };

    t.smash_down_start    = { State::Charge, 1u, &a.action_smash_down_start, &a.action_smash_down_charge };
    t.smash_forward_start = { State::Charge, 1u, &a.action_smash_forward_start, &a.action_smash_forward_charge };
    t.smash_up_start      = { State::Charge, 1u, &a.action_smash_up_start, &a.action_smash_up_charge };

    t.smash_down_attack    = { State::Attack, 1u, &a.action_smash_down_attack, nullptr };
    t.smash_forward_attack = { State::Attack, 1u, &a.action_smash_forward_attack, nullptr };
    t.smash_up_attack      = { State::Attack, 1u, &a.action_smash_up_attack, nullptr };
}

//============================================================================//

void PrivateFighter::initialise_hurt_blobs(const string& path)
{
    auto& hurtBlobs = fighter.mHurtBlobs;
    auto& fightWorld = fighter.mFightWorld;

    //--------------------------------------------------------//

    for (const auto& item : sq::parse_json_from_file(path + "/HurtBlobs.json"))
    {
        auto& blob = *hurtBlobs.emplace_back(fightWorld.create_hurt_blob(fighter));

        blob.bone = int8_t(item[0]);
        blob.originA = Vec3F(item[1], item[2], item[3]);
        blob.originB = Vec3F(item[4], item[5], item[6]);
        blob.radius = float(item[7]);
    }
}

//============================================================================//

void PrivateFighter::initialise_stats(const string& path)
{
    Fighter::Stats& stats = fighter.stats;

    const auto json = sq::parse_json_from_file(path + "/Stats.json");

    stats.walk_speed     = json.at("walk_speed");
    stats.dash_speed     = json.at("dash_speed");
    stats.air_speed      = json.at("air_speed");
    stats.traction       = json.at("traction");
    stats.air_mobility   = json.at("air_mobility");
    stats.air_friction   = json.at("air_friction");
    stats.hop_height     = json.at("hop_height");
    stats.jump_height    = json.at("jump_height");
    stats.air_hop_height = json.at("air_hop_height");
    stats.gravity        = json.at("gravity");
    stats.fall_speed     = json.at("fall_speed");
    stats.evade_distance = json.at("evade_distance");

    stats.dodge_finish     = json.at("dodge_finish");
    stats.dodge_safe_start = json.at("dodge_safe_start");
    stats.dodge_safe_end   = json.at("dodge_safe_end");

    stats.evade_finish     = json.at("evade_finish");
    stats.evade_safe_start = json.at("evade_safe_start");
    stats.evade_safe_end   = json.at("evade_safe_end");

    stats.air_dodge_finish     = json.at("air_dodge_finish");
    stats.air_dodge_safe_start = json.at("air_dodge_safe_start");
    stats.air_dodge_safe_end   = json.at("air_dodge_safe_end");
}

//============================================================================//

void PrivateFighter::initialise_actions(const string& path)
{
    Fighter::Actions& actions = fighter.actions;
    FightWorld& world = fighter.mFightWorld;

    // todo: surely these ctors don't need all this crap

    actions.neutral_first   = std::make_unique<Action>(world, fighter, Action::Type::Neutral_First);
    actions.tilt_down       = std::make_unique<Action>(world, fighter, Action::Type::Tilt_Down);
    actions.tilt_forward    = std::make_unique<Action>(world, fighter, Action::Type::Tilt_Forward);
    actions.tilt_up         = std::make_unique<Action>(world, fighter, Action::Type::Tilt_Up);
    actions.air_back        = std::make_unique<Action>(world, fighter, Action::Type::Air_Back);
    actions.air_down        = std::make_unique<Action>(world, fighter, Action::Type::Air_Down);
    actions.air_forward     = std::make_unique<Action>(world, fighter, Action::Type::Air_Forward);
    actions.air_neutral     = std::make_unique<Action>(world, fighter, Action::Type::Air_Neutral);
    actions.air_up          = std::make_unique<Action>(world, fighter, Action::Type::Air_Up);
    actions.dash_attack     = std::make_unique<Action>(world, fighter, Action::Type::Dash_Attack);
    actions.smash_down      = std::make_unique<Action>(world, fighter, Action::Type::Smash_Down);
    actions.smash_forward   = std::make_unique<Action>(world, fighter, Action::Type::Smash_Forward);
    actions.smash_up        = std::make_unique<Action>(world, fighter, Action::Type::Smash_Up);
    actions.special_down    = std::make_unique<Action>(world, fighter, Action::Type::Special_Down);
    actions.special_forward = std::make_unique<Action>(world, fighter, Action::Type::Special_Forward);
    actions.special_neutral = std::make_unique<Action>(world, fighter, Action::Type::Special_Neutral);
    actions.special_up      = std::make_unique<Action>(world, fighter, Action::Type::Special_Up);
}

//============================================================================//

void PrivateFighter::handle_input_movement(const Controller::Input& input)
{
    const Stats& stats = fighter.stats;

    State& state = fighter.current.state;
    Facing& facing = fighter.current.facing;
    Vec2F& velocity = fighter.mVelocity;

    //--------------------------------------------------------//

    mMoveAxisX = input.int_axis.x;
    mMoveAxisY = input.int_axis.y;

    //--------------------------------------------------------//

    const auto start_jump = [&]()
    {
        mJumpHeld = true;
    };

    const auto start_air_hop = [&]()
    {
        mJumpHeld = false;

        const float height = stats.air_hop_height * STS_AIR_HOP_HEIGHT;
        const float gravity = stats.gravity * STS_GRAVITY;

        velocity.y = std::sqrt(2.f * height * gravity * STS_TICK_RATE);
        velocity.y += gravity * 0.5f;
    };

    //--------------------------------------------------------//

    if (input.press_jump == true)
    {
        SWITCH (state) {
            CASE ( Walking, Dashing ) start_jump(); // moving jump
            CASE ( Neutral, Crouch, Brake, Shield ) start_jump(); // standing jump
            CASE ( Jumping, Falling ) start_air_hop(); // aerial jump
            CASE_DEFAULT {} // no jump allowed
        } SWITCH_END;
    }

    else if (input.hold_shield == true)
    {
        if (state == State::Shield && input.mash_axis.x != 0)
            facing = Facing(-input.mash_axis.x);
    }

    else if (state == State::Neutral)
    {
        if (input.float_axis.x < -0.0f) facing = Facing::Left;
        if (input.float_axis.x > +0.0f) facing = Facing::Right;
    }


    //-- transitions in response to input --------------------//

    DISABLE_FLOAT_EQUALITY_WARNING;

    SWITCH ( state ) {

    CASE ( Neutral ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_jump);

        else if (input.hold_shield == true)
            state_transition(transitions.misc_shield);

        else if (input.float_axis.y == -1.0f)
            state_transition(transitions.neutral_crouch);

        else if (input.float_axis.x != 0.0f)
            state_transition(transitions.neutral_walking);
    }

    CASE ( Walking ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_jump);

        else if (input.press_shield == true)
            state_transition(transitions.misc_shield);

        else if (input.float_axis.y == -1.0f)
            state_transition(transitions.walking_crouch);

        else if (input.float_axis.x == 0.0f)
            state_transition(transitions.walking_neutral);

        else if (input.mash_axis.x != 0)
            state_transition(transitions.walking_dashing);
    }

    CASE ( Dashing ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_jump);

        else if (input.press_shield == true)
            state_transition(transitions.misc_shield);

        else if (input.float_axis.x == 0.0f)
            state_transition(transitions.dashing_brake);
    }

    CASE ( Brake ) //=========================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_jump);
    }

    CASE ( Crouch ) //========================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_jump);

        if (input.press_shield == true)
            state_transition(transitions.misc_shield);

        else if (input.float_axis.y != -1.f)
            state_transition(transitions.crouch_stand);
    }

    CASE ( PreJump ) //=======================================//
    {
        mJumpHeld &= input.hold_jump;
    }

    CASE ( Jumping ) //=======================================//
    {
        mJumpHeld &= input.hold_jump;

        if (input.press_jump == true)
            state_transition(transitions.jumping_hop);

        else if (input.press_shield == true)
            state_transition(transitions.jumping_dodge);
    }

    CASE ( Falling ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.falling_hop);

        else if (input.press_shield == true)
            state_transition(transitions.falling_dodge);
    }

    CASE ( Shield ) //========================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_jump);

        else if (input.hold_shield == false)
            state_transition(transitions.shield_neutral);

        else if (input.mash_axis.x != 0)
            state_transition(transitions.shield_evade);

        else if (input.mash_axis.y != 0)
            state_transition(transitions.shield_dodge);
    }

    //== Nothing to do here ==================================//

    CASE ( Attack, AirAttack, Special, AirSpecial ) {}
    CASE ( Landing, Charge, Dodge, Evade, AirDodge ) {}

    //== Not Yet Implemented =================================//

    CASE ( Knocked, Stunned ) {}

    //--------------------------------------------------------//

    } SWITCH_END;

    ENABLE_FLOAT_EQUALITY_WARNING;

    //-- add horizontal velocity -----------------------------//

    const float walkMobility = stats.traction * STS_WALK_MOBILITY;
    const float dashMobility = stats.traction * STS_DASH_MOBILITY;
    const float landFriction = stats.traction * STS_LAND_FRICTION;

    const float airMobility = stats.air_mobility * STS_AIR_MOBILITY;
    const float airFriction = stats.air_friction * STS_AIR_FRICTION;

    const float baseWalkSpeed = stats.walk_speed * STS_WALK_SPEED;
    const float baseDashSpeed = stats.dash_speed * STS_DASH_SPEED;
    const float baseAirSpeed = stats.air_speed * STS_AIR_SPEED;

    const float walkTargetSpeed = std::abs(input.float_axis.x) * baseWalkSpeed + landFriction;
    const float dashTargetSpeed = std::abs(input.float_axis.x) * baseDashSpeed + landFriction;
    const float airTargetSpeed = std::abs(input.float_axis.x) * baseAirSpeed + airFriction;

    const auto apply_horizontal_move = [&](float mobility, float targetSpeed)
    {
        if (std::abs(velocity.x) >= targetSpeed) return;

        velocity.x += std::copysign(mobility, input.float_axis.x);
        velocity.x = maths::clamp_magnitude(velocity.x, targetSpeed);
    };

    if (state == State::Walking) apply_horizontal_move(walkMobility, walkTargetSpeed);
    if (state == State::Dashing) apply_horizontal_move(dashMobility, dashTargetSpeed);
    if (state == State::Jumping) apply_horizontal_move(airMobility, airTargetSpeed);
    if (state == State::Falling) apply_horizontal_move(airMobility, airTargetSpeed);
    if (state == State::AirAttack) apply_horizontal_move(airMobility, airTargetSpeed);
    if (state == State::AirSpecial) apply_horizontal_move(airMobility, airTargetSpeed);
    if (state == State::AirDodge) apply_horizontal_move(airMobility, airTargetSpeed);
}

//============================================================================//

void PrivateFighter::handle_input_actions(const Controller::Input& input)
{
    const State state = fighter.current.state;
    const Facing facing = fighter.current.facing;
    Action* const action = fighter.current.action;

    //--------------------------------------------------------//

    DISABLE_FLOAT_EQUALITY_WARNING;

    SWITCH ( state ) {

    //--------------------------------------------------------//

    CASE ( Neutral ) //=======================================//
    {
        if (input.press_attack == false) return;

        if      (input.mod_axis.y == -1) switch_action(Action::Type::Smash_Down);
        else if (input.mod_axis.y == +1) switch_action(Action::Type::Smash_Up);

        else if (input.float_axis.y < -0.f) switch_action(Action::Type::Tilt_Down);
        else if (input.float_axis.y > +0.f) switch_action(Action::Type::Tilt_Up);

        else switch_action(Action::Type::Neutral_First);
    }

    CASE ( Walking ) //=======================================//
    {
        if (input.press_attack == false) return;

        // todo: work out correct priority here

        if (input.mod_axis.x != 0)
            switch_action(Action::Type::Smash_Forward);

        else if (input.float_axis.y > std::abs(input.float_axis.x))
            switch_action(Action::Type::Tilt_Up);

        else switch_action(Action::Type::Tilt_Forward);
    }

    CASE ( Dashing ) //=======================================//
    {
        if (input.press_attack == false) return;

        if (input.mod_axis.x != 0) switch_action(Action::Type::Smash_Forward);

        else switch_action(Action::Type::Dash_Attack);
    }

    CASE ( Brake ) //=========================================//
    {
        if (input.press_attack == false) return;

        if (input.mod_axis.y == +1) switch_action(Action::Type::Smash_Up);

        else switch_action(Action::Type::Dash_Attack);
    }

    CASE ( Crouch ) //========================================//
    {
        if (input.press_attack == false) return;

        if (input.mod_axis.y == -1) switch_action(Action::Type::Smash_Down);

        else switch_action(Action::Type::Tilt_Down);
    }

    CASE ( Jumping, Falling ) //==============================//
    {
        if (input.press_attack == false) return;

        // todo: work out correct priority here

        if (input.float_axis.x == 0.f && input.float_axis.y == 0.f)
            switch_action(Action::Type::Air_Neutral);

        else if (std::abs(input.float_axis.x) >= std::abs(input.float_axis.y))
        {
            if (std::signbit(float(facing)) == std::signbit(input.float_axis.x))
                switch_action(Action::Type::Air_Forward);

            else switch_action(Action::Type::Air_Back);
        }

        else if (std::signbit(input.float_axis.y) == true)
            switch_action(Action::Type::Air_Down);

        else switch_action(Action::Type::Air_Up);
    }

    CASE ( Charge ) //========================================//
    {
        if (input.hold_attack == false) // todo: or max charge reached
        {
            if (action == fighter.actions.smash_down.get())
                state_transition(transitions.smash_down_attack);

            else if (action == fighter.actions.smash_forward.get())
                state_transition(transitions.smash_forward_attack);

            else if (action == fighter.actions.smash_up.get())
                state_transition(transitions.smash_up_attack);

            else SQASSERT(false, "invalid charge action");

            action->do_start();
        }
    }

    CASE ( Attack, AirAttack ) //=============================//
    {
        // will want to angle some attacks
    }

    CASE ( Special, AirSpecial ) //===========================//
    {
        // will want to share most input data here
    }

    //== Nothing to do here ==================================//

    CASE ( PreJump, Landing, Knocked, Stunned ) {}
    CASE ( Shield, Dodge, Evade, AirDodge ) {}

    //--------------------------------------------------------//

    } SWITCH_END;

    ENABLE_FLOAT_EQUALITY_WARNING;
}

//============================================================================//

void PrivateFighter::state_transition(const Transition& transition)
{
    fighter.current.state = transition.newState;

    mStateProgress = 0u;

    mAnimation = transition.animation;
    mNextAnimation = transition.loop;
    mFadeFrames = transition.fadeFrames;

    mStaticPose = nullptr;

    mAnimTimeDiscrete = 0u;
    mAnimTimeContinuous = 0.f;
    mFadeProgress = 0u;

    mFadeStartPose = current.pose;
}

//============================================================================//

void PrivateFighter::switch_action(Action::Type actionType)
{
    Action* const newAction = fighter.get_action(actionType);

    SQASSERT(newAction != fighter.current.action, "switch to same action");

    //--------------------------------------------------------//

    SWITCH ( actionType ) {

    //--------------------------------------------------------//

    CASE ( Neutral_First ) state_transition({ State::Attack, 1u, &animations.action_neutral_first });

    CASE ( Tilt_Down )    state_transition({ State::Attack, 1u, &animations.action_tilt_down });
    CASE ( Tilt_Forward ) state_transition({ State::Attack, 1u, &animations.action_tilt_forward });
    CASE ( Tilt_Up )      state_transition({ State::Attack, 1u, &animations.action_tilt_up });

    CASE ( Air_Back )    state_transition({ State::AirAttack, 1u, &animations.action_air_back });
    CASE ( Air_Down )    state_transition({ State::AirAttack, 1u, &animations.action_air_down });
    CASE ( Air_Forward ) state_transition({ State::AirAttack, 1u, &animations.action_air_forward });
    CASE ( Air_Neutral ) state_transition({ State::AirAttack, 1u, &animations.action_air_neutral });
    CASE ( Air_Up )      state_transition({ State::AirAttack, 1u, &animations.action_air_up });

    CASE ( Dash_Attack ) state_transition({ State::Attack, 1u, &animations.action_dash_attack });

    CASE ( Smash_Down )    state_transition(transitions.smash_down_start);
    CASE ( Smash_Forward ) state_transition(transitions.smash_forward_start);
    CASE ( Smash_Up )      state_transition(transitions.smash_up_start);

    CASE ( Special_Down )    {} // not sure
    CASE ( Special_Forward ) {} // not sure
    CASE ( Special_Neutral ) {} // not sure
    CASE ( Special_Up )      {} // not sure

    //--------------------------------------------------------//

    CASE ( None )
    {
        // we know here that current action is not nullptr

        // todo: this is a mess and is wrong

        SWITCH ( fighter.current.action->get_type() ) {

        CASE ( Neutral_First, Tilt_Forward, Tilt_Up, Smash_Down, Smash_Forward, Smash_Up, Dash_Attack )
        state_transition(transitions.misc_neutral);

        CASE ( Tilt_Down )
        state_transition(transitions.misc_crouch);

        CASE ( Air_Back, Air_Down, Air_Forward, Air_Neutral, Air_Up )
        state_transition(transitions.misc_falling);

        CASE ( Special_Down, Special_Forward, Special_Neutral, Special_Up )
        state_transition(transitions.misc_neutral); // todo

        CASE ( None ) SQASSERT(false, "switch from None to None");

        } SWITCH_END;
    }

    //--------------------------------------------------------//

    } SWITCH_END;

    //--------------------------------------------------------//

    if (newAction != nullptr) newAction->do_start();

    fighter.current.action = newAction;
}

//============================================================================//

void PrivateFighter::update_after_input()
{
    const Stats& stats = fighter.stats;

    Stage& stage = fighter.mFightWorld.get_stage();

    State& state = fighter.current.state;
    Facing& facing = fighter.current.facing;

    Status& status = fighter.status;

    Vec2F& velocity = fighter.mVelocity;

    auto& localDiamond = fighter.mLocalDiamond;
    auto& worldDiamond = fighter.mWorldDiamond;

    //-- update physics diamond to previous frame ------------//

    worldDiamond.negX = current.position.x - localDiamond.halfWidth;
    worldDiamond.posX = current.position.x + localDiamond.halfWidth;
    worldDiamond.negY = current.position.y;
    worldDiamond.posY = current.position.y + localDiamond.offsetTop;
    worldDiamond.crossX = current.position.x;
    worldDiamond.crossY = current.position.y + localDiamond.offsetMiddle;

    //-- apply friction --------------------------------------//

    SWITCH ( state ) {

    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Charge, Attack, Special, Landing, Shield, Dodge )
    {
        const float friction = stats.traction * STS_LAND_FRICTION;

        if (velocity.x < -0.f) velocity.x = maths::min(velocity.x + friction, -0.f);
        if (velocity.x > +0.f) velocity.x = maths::max(velocity.x - friction, +0.f);
    }

    CASE ( Jumping, Falling, AirAttack, AirSpecial, AirDodge )
    {
        const float friction = stats.air_friction * STS_AIR_FRICTION;

        if (velocity.x < -0.f) velocity.x = maths::min(velocity.x + friction, -0.f);
        if (velocity.x > +0.f) velocity.x = maths::max(velocity.x - friction, +0.f);
    }

    CASE ( Evade, PreJump, Knocked, Stunned ) {}

    } SWITCH_END;

    //-- perform passive state updates -----------------------//

    constexpr auto in_range = [](uint value, uint min, uint max) { return value >= min && value < max; };

    if (state == State::Evade)
    {
        status.intangible = in_range(++mStateProgress, stats.evade_safe_start, stats.evade_safe_end);

        if (mStateProgress <= stats.evade_finish)
        {
            const float distance = stats.evade_distance * STS_EVADE_DISTANCE;
            const float speed = distance / float(stats.evade_finish);

            velocity.x = speed * -float(facing) * 48.f;
        }

        else { state = State::Neutral; velocity.x = 0.f; }
    }

    else if (state == State::Dodge)
    {
        status.intangible = in_range(++mStateProgress, stats.dodge_safe_start, stats.dodge_safe_end);
        if (mStateProgress > stats.dodge_finish) state = State::Neutral;
    }

    else if (state == State::AirDodge)
    {
        status.intangible = in_range(++mStateProgress, stats.air_dodge_safe_start, stats.air_dodge_safe_end);
        if (mStateProgress > stats.air_dodge_finish) state = State::Neutral;
    }

    else if (state == State::Landing)
    {
        if (++mStateProgress == STS_LANDING_LAG) state = State::Neutral;
    }

    else if (state == State::PreJump)
    {
        if (++mStateProgress == STS_JUMP_DELAY)
        {
            const float hopHeight = stats.hop_height * STS_HOP_HEIGHT;
            const float jumpHeight = stats.jump_height * STS_JUMP_HEIGHT;
            const float gravity = stats.gravity * STS_GRAVITY;

            if (mJumpHeld == true) velocity.y = std::sqrt(2.f * jumpHeight * gravity * STS_TICK_RATE);
            if (mJumpHeld == false) velocity.y = std::sqrt(2.f * hopHeight * gravity * STS_TICK_RATE);

            state = State::Jumping;
        }
    }

    else if (state == State::Jumping)
    {
        if (velocity.y <= 0.f)
            state_transition(transitions.jumping_falling);
    }

    else if (state == State::Brake)
    {
        if (velocity.x == 0.f)
            state = State::Neutral;
    }

    else if (state == State::Knocked)
    {

    }

    //-- apply gravity ---------------------------------------//

    velocity.y -= stats.gravity * STS_GRAVITY;

    velocity.y = maths::max(velocity.y, stats.fall_speed * -STS_FALL_SPEED);

    //-- update position -------------------------------------//

    const Vec2F targetPosition = current.position + velocity / 48.f;

    MoveAttempt moveAttempt = stage.attempt_move(worldDiamond, velocity / 48.f);

    if (moveAttempt.edge != 0)
    {
        if (state == State::Evade || state == State::Attack || state == State::Special)
            current.position = moveAttempt.result;

        else if (state == State::Knocked || state == State::Jumping || std::abs(mMoveAxisX) == 2)
            current.position = targetPosition;
    }

    else current.position = moveAttempt.result;

    //-- check if fallen or moved off an edge ----------------//

    if (current.position.y <= targetPosition.y)
    {
        SWITCH (state) {
            CASE (Walking) { state_transition(transitions.walking_dive); }
            CASE (Dashing) { state_transition(transitions.dashing_dive); }
            CASE (Special) {} // todo
            CASE (PreJump, Jumping, Falling, AirAttack, AirSpecial, AirDodge, Knocked) {}
            CASE_DEFAULT { state_transition(transitions.misc_falling); }
        } SWITCH_END;
    }

    //-- check if landed on the ground -----------------------//

    if (current.position.y > targetPosition.y)
    {
        if (state == State::AirDodge) state_transition(transitions.misc_land);
        else if (state == State::Jumping) state_transition(transitions.misc_land);
        else if (state == State::Falling) state_transition(transitions.misc_land);

        else if (state == State::AirAttack)
        {
            state_transition(transitions.misc_land);
            switch_action(Action::Type::None);
        }

        else if (state == State::AirSpecial)
        {
            // todo
        }

        else if (state == State::Knocked)
        {
            if (maths::length(velocity) > 5.f)
            {
                // todo
            }

            state_transition(transitions.misc_land);
        }
    }

    //-- check if walls, ceiling, or floor reached -----------//

    if (current.position.x > targetPosition.x || current.position.x < targetPosition.x)
    {
        state_transition(transitions.misc_neutral);
        velocity.x = 0.f;
    }

    if (current.position.y < targetPosition.y) velocity.y = 0.f;
    if (current.position.y > targetPosition.y) velocity.y = 0.f;

    //--------------------------------------------------------//

    if (state == State::Neutral && velocity.x == 0.f && moveAttempt.edge == int8_t(facing))
        state_transition(transitions.misc_vertigo);

    //-- update the active action ----------------------------//

    if (fighter.current.action != nullptr)
        if (fighter.current.state != State::Charge)
            if (fighter.current.action->do_tick() == true)
                switch_action(Action::Type::None);
}

//============================================================================//

void PrivateFighter::base_tick_fighter()
{
    fighter.previous = fighter.current;

    previous = current;

    //--------------------------------------------------------//

    SQASSERT(controller != nullptr, "");

    const auto input = controller->get_input();

    //--------------------------------------------------------//

    handle_input_movement(input);

    handle_input_actions(input);

    update_after_input();

    //--------------------------------------------------------//

    const Vec3F position ( current.position, 0.f );
    const QuatF rotation ( 0.f, 0.25f * float(fighter.current.facing), 0.f );

    fighter.mModelMatrix = maths::transform(position, rotation, Vec3F(1.f));
}

//============================================================================//

void PrivateFighter::base_tick_animation()
{
    SQASSERT(bool(mAnimation) != bool(mStaticPose), "XOR");

    //--------------------------------------------------------//

    // update these animations based on horizontal velocity
    if ( mAnimation == &animations.walking_loop || mAnimation == &animations.dashing_loop )
    {
        mAnimTimeContinuous += std::abs(fighter.mVelocity.x);

        current.pose = armature.compute_pose(*mAnimation, mAnimTimeContinuous);
    }

    // update all other animations based on time
    else if (mAnimation != nullptr)
    {
        current.pose = armature.compute_pose(*mAnimation, float(mAnimTimeDiscrete));

        if (++mAnimTimeDiscrete >= int(mAnimation->totalTime))
        {
            if (mAnimation->times.back() == 0u)
            {
                if (mNextAnimation == nullptr)
                    mStaticPose = &mAnimation->poses.back();

                mAnimation = mNextAnimation;
                mNextAnimation = nullptr;

                mAnimTimeDiscrete = 0;
                mAnimTimeContinuous = 0.f;
            }
            else mAnimTimeDiscrete = 0;
        }
    }

    // if we haven't an animation, we have a static pose
    else if (mStaticPose != nullptr)
    {
        // todo: only need to copy this once
        current.pose = *mStaticPose;
    }

    //--------------------------------------------------------//

    // blend in old pose for basic transitions
    if (mFadeProgress != mFadeFrames)
    {
        const float blend = float(++mFadeProgress) / float(mFadeFrames + 1u);
        current.pose = armature.blend_poses(mFadeStartPose, current.pose, blend);
    }

    fighter.mBoneMatrices = armature.compute_ubo_data(current.pose);
}
