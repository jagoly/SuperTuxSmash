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

static constexpr float STS_TICK_RATE        = 48.0f;
static constexpr int   STS_LANDING_LAG      = 4;
static constexpr int   STS_JUMP_DELAY       = 5;

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
    load_animation ( a.walking_loop, "WalkingLoop" );

    load_animation ( a.airhop, "Airhop" );
    load_animation ( a.brake,  "Brake"  );
    load_animation ( a.crouch, "Crouch" );
    load_animation ( a.jump,   "Jump"   );
    load_animation ( a.land,   "Land"   );
    load_animation ( a.stand,  "Stand"  );

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
    t.neutral_jump    = { State::PreJump, 1u, &a.jump, &a.jumping_loop };
    t.neutral_walking = { State::Walking, 4u, &a.walking_loop, nullptr };

    t.walking_crouch  = { State::Crouch,  2u, &a.crouch, &a.crouch_loop };
    t.walking_jump    = { State::PreJump, 1u, &a.jump, &a.jumping_loop };
    t.walking_dashing = { State::Dashing, 4u, &a.dashing_loop, nullptr };
    t.walking_neutral = { State::Neutral, 4u, &a.neutral_loop, nullptr };

    t.dashing_jump  = { State::PreJump, 1u, &a.jump, &a.jumping_loop };
    t.dashing_brake = { State::Brake,   2u, &a.brake, &a.neutral_loop };

    t.crouch_jump  = { State::PreJump, 1u, &a.jump, &a.jumping_loop };
    t.crouch_stand = { State::Neutral, 2u, &a.stand, &a.neutral_loop };

    t.jumping_hop  = { State::Jumping, 1u, &a.airhop, &a.jumping_loop };
    t.jumping_fall = { State::Falling, 8u, &a.falling_loop, nullptr };

    t.falling_hop  = { State::Jumping, 1u, &a.airhop, &a.jumping_loop };
    t.falling_land = { State::Landing, 1u, &a.land, &a.neutral_loop };

    t.other_fall = { State::Falling, 4u, &a.falling_loop, nullptr };

    t.smash_down_start    = { State::Charge, 1u, &a.action_smash_down_start, &a.action_smash_down_charge };
    t.smash_forward_start = { State::Charge, 1u, &a.action_smash_forward_start, &a.action_smash_forward_charge };
    t.smash_up_start      = { State::Charge, 1u, &a.action_smash_up_start, &a.action_smash_up_charge };

    t.smash_down_attack    = { State::Attack, 1u, &a.action_smash_down_attack, nullptr };
    t.smash_forward_attack = { State::Attack, 1u, &a.action_smash_forward_attack, nullptr };
    t.smash_up_attack      = { State::Attack, 1u, &a.action_smash_up_attack, nullptr };

    t.attack_to_neutral = { State::Neutral, 2u, &a.neutral_loop, nullptr };
    t.attack_to_crouch  = { State::Crouch,  2u, &a.crouch_loop, nullptr };
    t.attack_to_falling = { State::Falling, 2u, &a.falling_loop, nullptr };
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
}

//============================================================================//

void PrivateFighter::handle_input_movement(const Controller::Input& input)
{
    const Stats& stats = fighter.stats;

    State& state = fighter.state;
    Facing& facing = fighter.facing;
    Vec2F& velocity = fighter.mVelocity;

    //--------------------------------------------------------//

    const auto start_jump = [&]()
    {
        mJumpHeld = true;

        mJumpDelay = STS_JUMP_DELAY;
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
            CASE ( Neutral, Crouch, Brake ) start_jump(); // standing jump
            CASE ( Jumping, Falling ) start_air_hop(); // aerial jump
            CASE_DEFAULT {} // no jump allowed
        } SWITCH_END;
    }

    if (state == State::Neutral)
    {
        if (input.axis_move.x < -0.0f) facing = Facing::Left;
        if (input.axis_move.x > +0.0f) facing = Facing::Right;
    }


    //-- transitions in response to input --------------------//

    DISABLE_FLOAT_EQUALITY_WARNING;

    SWITCH ( state ) {

    CASE ( Neutral ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.neutral_jump);

        else if (input.axis_move.y == -1.0f)
            state_transition(transitions.neutral_crouch);

        else if (input.axis_move.x != 0.0f)
            state_transition(transitions.neutral_walking);
    }

    CASE ( Walking ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.walking_jump);

        else if (input.axis_move.y == -1.0f)
            state_transition(transitions.walking_crouch);

        else if (input.axis_move.x == 0.0f)
            state_transition(transitions.walking_neutral);

        else if (input.mash_axis_x != 0)
            state_transition(transitions.walking_dashing);
    }

    CASE ( Dashing ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.dashing_jump);

        else if (input.axis_move.x == 0.0f)
            state_transition(transitions.dashing_brake);
    }

    CASE ( Brake ) //=========================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.neutral_jump);
    }

    CASE ( Crouch ) //========================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.crouch_jump);

        else if (input.axis_move.y != -1.f)
            state_transition(transitions.crouch_stand);
    }

    CASE ( Landing ) //=======================================//
    {
        if (--mLandingLag == 0u)
        {
            state = State::Neutral;
        }
    }

    CASE ( PreJump ) //=======================================//
    {
        mJumpHeld &= input.hold_jump;

        if (--mJumpDelay == 0u)
        {
            const float hopHeight = stats.hop_height * STS_HOP_HEIGHT;
            const float jumpHeight = stats.jump_height * STS_JUMP_HEIGHT;
            const float gravity = stats.gravity * STS_GRAVITY;

            if (mJumpHeld == true) velocity.y = std::sqrt(2.f * jumpHeight * gravity * STS_TICK_RATE);
            if (mJumpHeld == false) velocity.y = std::sqrt(2.f * hopHeight * gravity * STS_TICK_RATE);

            state = State::Jumping;
        }
    }

    CASE ( Jumping ) //=======================================//
    {
        mJumpHeld &= input.hold_jump;

        if (input.press_jump == true)
            state_transition(transitions.jumping_hop);

        else if (velocity.y <= 0.f)
            state_transition(transitions.jumping_fall);
    }

    CASE ( Falling ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.falling_hop);
    }

    //== Not Yet Implemented =================================//

    CASE ( Charge, Attack, AirAttack, Knocked, Stunned ) {}

    //--------------------------------------------------------//

    } SWITCH_END;

    ENABLE_FLOAT_EQUALITY_WARNING;

    //-- add horizontal velocity -----------------------------//

    // todo: this could definitely be cleaner...

    const float walkMobility = stats.traction * STS_WALK_MOBILITY;
    const float dashMobility = stats.traction * STS_DASH_MOBILITY;
    const float landFriction = stats.traction * STS_LAND_FRICTION;

    const float airMobility = stats.air_mobility * STS_AIR_MOBILITY;
    const float airFriction = stats.air_friction * STS_AIR_FRICTION;

    const float baseWalkSpeed = stats.walk_speed * STS_WALK_SPEED;
    const float baseDashSpeed = stats.dash_speed * STS_DASH_SPEED;
    const float baseAirSpeed = stats.air_speed * STS_AIR_SPEED;

    const float walkTargetSpeed = std::abs(input.axis_move.x) * baseWalkSpeed + landFriction;
    const float dashTargetSpeed = std::abs(input.axis_move.x) * baseDashSpeed + landFriction;
    const float airTargetSpeed = std::abs(input.axis_move.x) * baseAirSpeed + airFriction;

    const auto apply_horizontal_move = [&](float mobility, float targetSpeed)
    {
        if (std::abs(velocity.x) >= targetSpeed) return;

        velocity.x += std::copysign(mobility, input.axis_move.x);
        velocity.x = maths::clamp_magnitude(velocity.x, targetSpeed);
    };

    if (state == State::Walking) apply_horizontal_move(walkMobility, walkTargetSpeed);
    if (state == State::Dashing) apply_horizontal_move(dashMobility, dashTargetSpeed);
    if (state == State::Jumping) apply_horizontal_move(airMobility, airTargetSpeed);
    if (state == State::Falling) apply_horizontal_move(airMobility, airTargetSpeed);
}

//============================================================================//

void PrivateFighter::handle_input_actions(const Controller::Input& input)
{
    if (mActiveAction != nullptr) return;

    //--------------------------------------------------------//

    State& state = fighter.state;
    Facing& facing = fighter.facing;

    Actions& actions = *fighter.mActions;

    //--------------------------------------------------------//

    if (state == State::Charge)
    {
        if (input.hold_attack == false) // todo: or max charge reached
        {
            if (mActionType == Action::Type::Smash_Down)
            {
                state_transition(transitions.smash_down_attack);
                mActiveAction = actions.smash_down.get();
            }
            else if (mActionType == Action::Type::Smash_Forward)
            {
                state_transition(transitions.smash_forward_attack);
                mActiveAction = actions.smash_forward.get();
            }
            else if (mActionType == Action::Type::Smash_Up)
            {
                state_transition(transitions.smash_up_attack);
                mActiveAction = actions.smash_up.get();
            }
            else SQASSERT(false, "");

            mActiveAction->do_start();
        }

        return;
    }

    //--------------------------------------------------------//

    if (input.press_attack == false) return;

    //--------------------------------------------------------//

    DISABLE_FLOAT_EQUALITY_WARNING;

    SWITCH ( state ) {

    //--------------------------------------------------------//

    CASE ( Neutral ) //=======================================//
    {
        if      (input.mod_axis_y == -1) switch_action(Action::Type::Smash_Down);
        else if (input.mod_axis_y == +1) switch_action(Action::Type::Smash_Up);

        else if (input.axis_move.y < -0.f) switch_action(Action::Type::Tilt_Down);
        else if (input.axis_move.y > +0.f) switch_action(Action::Type::Tilt_Up);

        else switch_action(Action::Type::Neutral_First);
    }

    CASE ( Walking ) //=======================================//
    {
        // todo: work out correct priority here

        if (input.mod_axis_x != 0)
            switch_action(Action::Type::Smash_Forward);

        else if (input.axis_move.y > std::abs(input.axis_move.x))
            switch_action(Action::Type::Tilt_Up);

        else switch_action(Action::Type::Tilt_Forward);
    }

    CASE ( Dashing ) //=======================================//
    {
        if (input.mod_axis_x != 0)
            switch_action(Action::Type::Smash_Forward);

        else switch_action(Action::Type::Dash_Attack);
    }

    CASE ( Brake ) //=========================================//
    {
        if (input.mod_axis_y == +1)
            switch_action(Action::Type::Smash_Up);

        else switch_action(Action::Type::Dash_Attack);
    }

    CASE ( Crouch ) //========================================//
    {
        if (input.mod_axis_y == -1) switch_action(Action::Type::Smash_Down);

        else switch_action(Action::Type::Tilt_Down);
    }

    CASE ( Jumping, Falling ) //==============================//
    {
        // todo: work out correct priority here

        if (input.axis_move.x == 0.f && input.axis_move.y == 0.f)
            switch_action(Action::Type::Air_Neutral);

        else if (std::abs(input.axis_move.x) >= std::abs(input.axis_move.y))
        {
            if (std::signbit(float(facing)) == std::signbit(input.axis_move.x))
                switch_action(Action::Type::Air_Forward);

            else switch_action(Action::Type::Air_Back);
        }

        else if (std::signbit(input.axis_move.y) == true)
            switch_action(Action::Type::Air_Down);

        else switch_action(Action::Type::Air_Up);
    }

    CASE ( Landing, PreJump ) //==============================//
    {
        // can't do stuff afaik
    }

    CASE ( Knocked, Stunned ) //==============================//
    {
        // can't do stuff afaik
    }

    CASE ( Charge, Attack, AirAttack ) //=====================//
    {
        SQASSERT(false, "this shouldn't happen...");
    }

    //--------------------------------------------------------//

    } SWITCH_END;

    ENABLE_FLOAT_EQUALITY_WARNING;
}

//============================================================================//

void PrivateFighter::state_transition(const Transition& transition)
{
    fighter.state = transition.newState;

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
    SQASSERT(actionType != mActionType, "");

    //--------------------------------------------------------//

    Actions& actions = *fighter.mActions;

    //--------------------------------------------------------//

    SWITCH ( actionType ) {

    //--------------------------------------------------------//

    CASE ( Neutral_First )
    state_transition({ State::Attack, 1u, &animations.action_neutral_first });
    mActiveAction = actions.neutral_first.get();

    CASE ( Tilt_Down )
    state_transition({ State::Attack, 1u, &animations.action_tilt_down });
    mActiveAction = actions.tilt_down.get();

    CASE ( Tilt_Forward )
    state_transition({ State::Attack, 1u, &animations.action_tilt_forward });
    mActiveAction = actions.tilt_forward.get();

    CASE ( Tilt_Up )
    state_transition({ State::Attack, 1u, &animations.action_tilt_up });
    mActiveAction = actions.tilt_up.get();

    CASE ( Air_Back )
    state_transition({ State::AirAttack, 1u, &animations.action_air_back });
    mActiveAction = actions.air_back.get();

    CASE ( Air_Down )
    state_transition({ State::AirAttack, 1u, &animations.action_air_down });
    mActiveAction = actions.air_down.get();

    CASE ( Air_Forward )
    state_transition({ State::AirAttack, 1u, &animations.action_air_forward });
    mActiveAction = actions.air_forward.get();

    CASE ( Air_Neutral )
    state_transition({ State::AirAttack, 1u, &animations.action_air_neutral });
    mActiveAction = actions.air_neutral.get();

    CASE ( Air_Up )
    state_transition({ State::AirAttack, 1u, &animations.action_air_up });
    mActiveAction = actions.air_up.get();

    CASE ( Dash_Attack )
    state_transition({ State::Attack, 1u, &animations.action_dash_attack });
    mActiveAction = actions.dash_attack.get();

    CASE ( Smash_Down )
    state_transition(transitions.smash_down_start);

    CASE ( Smash_Forward )
    state_transition(transitions.smash_forward_start);

    CASE ( Smash_Up )
    state_transition(transitions.smash_up_start);

    CASE ( Special_Neutral )
    {} // not sure

    //--------------------------------------------------------//

    CASE ( None )
    {
        mActiveAction = nullptr;

        SWITCH ( mActionType ) {

        CASE ( Neutral_First, Tilt_Forward, Tilt_Up, Smash_Down, Smash_Forward, Smash_Up, Dash_Attack )
        state_transition(transitions.attack_to_neutral);

        CASE ( Tilt_Down )
        state_transition(transitions.attack_to_crouch);

        CASE ( Air_Back, Air_Down, Air_Forward, Air_Neutral, Air_Up )
        state_transition(transitions.attack_to_falling);

        CASE ( Special_Neutral )
        state_transition(transitions.attack_to_neutral); // todo

        CASE ( None ) SQASSERT(false, "switch from None to None");

        } SWITCH_END;
    }

    //--------------------------------------------------------//

    } SWITCH_END;

    //--------------------------------------------------------//

    if (mActiveAction != nullptr)
    {
        mActiveAction->do_start();
    }

    mActionType = actionType;
}

//============================================================================//

void PrivateFighter::update_physics()
{
    const Stats& stats = fighter.stats;

    Stage& stage = fighter.mFightWorld.get_stage();

    State& state = fighter.state;

    Vec2F& velocity = fighter.mVelocity;

    auto& localDiamond = fighter.mLocalDiamond;
    auto& worldDiamond = fighter.mWorldDiamond;

    //-- update physics diamond to previous frame ------------//

    worldDiamond.xNeg = localDiamond.xNeg + current.position;
    worldDiamond.xPos = localDiamond.xPos + current.position;
    worldDiamond.yNeg = localDiamond.yNeg + current.position;
    worldDiamond.yPos = localDiamond.yPos + current.position;

    //-- apply friction --------------------------------------//

    SWITCH ( state ) {

    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Charge, Attack, Landing )
    {
        const float friction = stats.traction * STS_LAND_FRICTION;

        if (velocity.x < -0.f) velocity.x = maths::min(velocity.x + friction, -0.f);
        if (velocity.x > +0.f) velocity.x = maths::max(velocity.x - friction, +0.f);
    }

    CASE ( Jumping, Falling, AirAttack )
    {
        const float friction = stats.air_friction * STS_AIR_FRICTION;

        if (velocity.x < -0.f) velocity.x = maths::min(velocity.x + friction, -0.f);
        if (velocity.x > +0.f) velocity.x = maths::max(velocity.x - friction, +0.f);
    }

    CASE ( PreJump )
    {
        // apply no friction
    }

    CASE ( Knocked, Stunned ) {}

    } SWITCH_END;


    //--------------------------------------------------------//

    if (state == State::Brake && velocity.x == 0.f)
    {
        state = State::Neutral;
    }


    //-- apply gravity ---------------------------------------//

    velocity.y -= stats.gravity * STS_GRAVITY;

    velocity.y = maths::max(velocity.y, stats.fall_speed * -STS_FALL_SPEED);


    //-- update position -------------------------------------//

    const Vec2F targetPosition = current.position + velocity / 48.f;

    current.position = stage.transform_response(current.position, worldDiamond, velocity / 48.f);


    //-- check if fallen or landed ---------------------------//

    SWITCH ( state ) {

    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Charge, Attack, Landing )
    {
        if (current.position.y <= targetPosition.y)
        {
            state_transition(transitions.other_fall);
        }
    }

    CASE ( PreJump )
    {
        // can't fall during pre-jump
    }

    CASE ( Jumping, Falling, AirAttack )
    {
        if (current.position.y > targetPosition.y)
        {
            if (state == State::AirAttack)
                switch_action(Action::Type::None);

            mLandingLag = STS_LANDING_LAG;

            state_transition(transitions.falling_land);
        }
    }

    CASE ( Knocked )
    {
        if (current.position.y > targetPosition.y)
        {
            if (maths::length(velocity) > 5.f)
            {
                // todo: do splat stuff
            }

            mLandingLag = STS_LANDING_LAG;

            state_transition(transitions.falling_land);
        }
    }

    CASE ( Stunned ) {}

    } SWITCH_END;


    //-- check if walls, ceiling, or floor reached -----------//

    if (current.position.x > targetPosition.x) velocity.x = 0.f;
    if (current.position.x < targetPosition.x) velocity.x = 0.f;
    if (current.position.y < targetPosition.y) velocity.y = 0.f;
    if (current.position.y > targetPosition.y) velocity.y = 0.f;
}

//============================================================================//

void PrivateFighter::update_active_action()
{
    if (mActiveAction != nullptr)
    {
        if (mActiveAction->do_tick() == true)
        {
            switch_action(Action::Type::None);
        }
    }
}

//============================================================================//

void PrivateFighter::base_tick_fighter()
{
    previous = current;

    //--------------------------------------------------------//

    SQASSERT(controller != nullptr, "");

    const auto input = controller->get_input();

    handle_input_movement(input);

    handle_input_actions(input);

    //--------------------------------------------------------//

    update_physics();

    update_active_action();

    //--------------------------------------------------------//

    const Vec3F position ( current.position, 0.f );
    const QuatF rotation ( 0.f, 0.25f * float(fighter.facing), 0.f );

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
