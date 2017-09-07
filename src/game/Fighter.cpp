#include <sqee/assert.hpp>
#include <sqee/debug/Logging.hpp>

#include <sqee/macros.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/maths/Functions.hpp>

#include "game/Stage.hpp"
#include "game/Fighter.hpp"

namespace maths = sq::maths;
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
static constexpr int   STS_JUMP_DELAY       = 3;

//============================================================================//

Fighter::Fighter(uint8_t index, FightWorld& world, Controller& controller, string path)
    : index(index), mFightWorld(world), mController(controller)
{
    impl_initialise_armature(path);
    impl_initialise_hurt_blobs(path);
    impl_initialise_stats(path);
}

Fighter::~Fighter() = default;

//============================================================================//

void Fighter::impl_initialise_armature(const string& path)
{
    mArmature.load_bones(path + "Bones.txt", true);
    mArmature.load_rest_pose(path + "RestPose.txt");

    current.pose = previous.pose = mArmature.get_rest_pose();

    //--------------------------------------------------------//

    const auto load_animation = [&](sq::Armature::Animation& anim, const char* name)
    {
        const string filePath = path + "anims/" + name + ".txt";

        if (sq::check_file_exists(filePath) == false)
        {
            sq::log_warning("missing animation '%s'", filePath);
            anim = mArmature.make_animation(path + "anims/Null.txt");
            return;
        }

        anim = mArmature.make_animation(path + "anims/" + name + ".txt");
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

    //--------------------------------------------------------//

    load_animation ( a.action_Neutral_First, "actions/Neutral_First" );

    load_animation ( a.action_Tilt_Down,    "actions/Tilt_Down"    );
    load_animation ( a.action_Tilt_Forward, "actions/Tilt_Forward" );
    load_animation ( a.action_Tilt_Up,      "actions/Tilt_Up"      );

    load_animation ( a.action_Air_Back,    "actions/Air_Back"    );
    load_animation ( a.action_Air_Down,    "actions/Air_Down"    );
    load_animation ( a.action_Air_Forward, "actions/Air_Forward" );
    load_animation ( a.action_Air_Neutral, "actions/Air_Neutral" );
    load_animation ( a.action_Air_Up,      "actions/Air_Up"      );

    load_animation ( a.action_Dash_Attack, "actions/Dash_Attack" );

    //--------------------------------------------------------//

    t.neutral_crouch    = { State::Crouch,  2u, &a.crouch, &a.crouch_loop };
    t.neutral_jump      = { State::PreJump, 1u, &a.jump, &a.jumping_loop };
    t.neutral_walking   = { State::Walking, 4u, &a.walking_loop, nullptr };

    t.walking_crouch    = { State::Crouch,  2u, &a.crouch, &a.crouch_loop };
    t.walking_jump      = { State::PreJump, 1u, &a.jump, &a.jumping_loop };
    t.walking_dashing   = { State::Dashing, 4u, &a.dashing_loop, nullptr };
    t.walking_neutral   = { State::Neutral, 4u, &a.neutral_loop, nullptr };

    t.dashing_jump      = { State::PreJump, 1u, &a.jump, &a.jumping_loop };
    t.dashing_brake     = { State::Brake,   2u, &a.brake, &a.neutral_loop };

    t.crouch_jump       = { State::PreJump, 1u, &a.jump, &a.jumping_loop };
    t.crouch_stand      = { State::Neutral, 2u, &a.stand, &a.neutral_loop };

    t.jumping_hop       = { State::Jumping, 0u, &a.airhop, &a.jumping_loop };
    t.jumping_fall      = { State::Falling, 8u, &a.falling_loop, nullptr };

    t.falling_hop       = { State::Jumping, 0u, &a.airhop, &a.jumping_loop };
    t.falling_land      = { State::Landing, 0u, &a.land, &a.neutral_loop };

    t.attack_to_neutral = { State::Neutral, 2u, &a.neutral_loop, nullptr };
    t.attack_to_crouch  = { State::Crouch,  2u, &a.crouch_loop, nullptr };
    t.attack_to_falling = { State::Falling, 2u, &a.falling_loop, nullptr };
}

//============================================================================//

void Fighter::impl_initialise_hurt_blobs(const string& path)
{
    for (const auto& item : sq::parse_json(path + "HurtBlobs.json"))
    {
        mHurtBlobs.push_back(mFightWorld.create_hurt_blob(*this));
        HurtBlob& blob = *mHurtBlobs.back();

        blob.bone = int8_t(item[0]);
        blob.originA = Vec3F(item[1], item[2], item[3]);
        blob.originB = Vec3F(item[4], item[5], item[6]);
        blob.radius = float(item[7]);
    }
}

//============================================================================//

void Fighter::impl_initialise_stats(const string& path)
{
    const auto json = sq::parse_json(path + "Stats.json");

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

void Fighter::impl_handle_input_movement(const Controller::Input& input)
{
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

        mVelocity.y = std::sqrt(2.f * height * gravity * STS_TICK_RATE);
        mVelocity.y += gravity * 0.5f;
    };

    const auto apply_horizontal_move = [&](float mobility, float targetSpeed)
    {
        if (std::abs(mVelocity.x) >= targetSpeed) return;

        mVelocity.x += std::copysign(mobility, input.axis_move.x);
        mVelocity.x = maths::clamp_magnitude(mVelocity.x, targetSpeed);
    };

    //--------------------------------------------------------//

    DISABLE_FLOAT_EQUALITY_WARNING;

    SWITCH ( state ) {

    //--------------------------------------------------------//

    CASE ( Neutral ) //=======================================//
    {
        if (input.press_jump == true)
        {
            start_jump();

            impl_state_transition(transitions.neutral_jump);
        }

        else if (input.axis_move.y == -1.0f)
        {
            impl_state_transition(transitions.neutral_crouch);
        }

        else if (input.axis_move.x != 0.0f)
        {
            facing = Facing { std::signbit(input.axis_move.x) ? -1 : +1 };

            apply_horizontal_move(walkMobility, walkTargetSpeed);

            impl_state_transition(transitions.neutral_walking);
        }
    }

    CASE ( Walking ) //=======================================//
    {
        if (input.press_jump == true)
        {
            start_jump();

            impl_state_transition(transitions.walking_jump);
        }

        else if (input.axis_move.y == -1.0f)
        {
            impl_state_transition(transitions.walking_crouch);
        }

        else if (input.axis_move.x == 0.0f)
        {
            impl_state_transition(transitions.walking_neutral);
        }

        else if (input.mash_neg_x || input.mash_pos_x)
        {
            apply_horizontal_move(dashMobility, dashTargetSpeed);

            impl_state_transition(transitions.walking_dashing);
        }

        else apply_horizontal_move(walkMobility, walkTargetSpeed);
    }

    CASE ( Dashing ) //=======================================//
    {
        if (input.press_jump == true)
        {
            start_jump();

            impl_state_transition(transitions.dashing_jump);
        }

        else if (input.axis_move.x == 0.0f)
        {
            impl_state_transition(transitions.dashing_brake);
        }

        else apply_horizontal_move(dashMobility, dashTargetSpeed);
    }

    CASE ( Brake ) //=========================================//
    {
        if (input.press_jump == true)
        {
            start_jump();

            impl_state_transition(transitions.neutral_jump);
        }

        else if (mVelocity.x == 0.f)
        {
            state = State::Neutral;
        }
    }

    CASE ( Crouch ) //========================================//
    {
        if (input.press_jump == true)
        {
            start_jump();

            impl_state_transition(transitions.crouch_jump);
        }

        else if (input.axis_move.y != -1.f)
        {
            impl_state_transition(transitions.crouch_stand);
        }
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

            mVelocity.y = std::sqrt(2.f * hopHeight * gravity * STS_TICK_RATE);

            if (mJumpHeld == false) mVelocity.y += gravity * 0.5f;
            else mVelocity.y += gravity - gravity * hopHeight / jumpHeight * 0.5f;

            state = State::Jumping;
        }
    }

    CASE ( Jumping ) //=======================================//
    {
        mJumpHeld &= input.hold_jump;

        apply_horizontal_move(airMobility, airTargetSpeed);

        if (input.press_jump == true)
        {
            start_air_hop();

            impl_state_transition(transitions.jumping_hop);
        }

        else if (mVelocity.y <= 0.f)
        {
            impl_state_transition(transitions.jumping_fall);

            //sq::log_info("%s", current.position);
        }

        else if (mJumpHeld == true)
        {
            const float hopHeight = stats.hop_height * STS_HOP_HEIGHT;
            const float jumpHeight = stats.jump_height * STS_JUMP_HEIGHT;
            const float gravity = stats.gravity * STS_GRAVITY;

            mVelocity.y += gravity - gravity * hopHeight / jumpHeight;
        }
    }

    CASE ( Falling ) //=======================================//
    {
        apply_horizontal_move(airMobility, airTargetSpeed);

        if (input.press_jump == true)
        {
            start_air_hop();

            impl_state_transition(transitions.falling_hop);
        }
    }

    //== Not Yet Implemented =================================//

    CASE ( Attack, AirAttack, Knocked, Stunned ) {}

    //--------------------------------------------------------//

    } SWITCH_END;

    ENABLE_FLOAT_EQUALITY_WARNING;
}

//============================================================================//

void Fighter::impl_handle_input_actions(const Controller::Input& input)
{
    if (mActiveAction != nullptr) return;
    if (input.press_attack == false) return;

    //--------------------------------------------------------//

    DISABLE_FLOAT_EQUALITY_WARNING;

    SWITCH ( state ) {

    //--------------------------------------------------------//

    CASE ( Neutral ) //=======================================//
    {
        if (input.axis_move.y < -0.f) impl_switch_action(Action::Type::Tilt_Down);
        else if (input.axis_move.y > +0.f) impl_switch_action(Action::Type::Tilt_Up);
        else impl_switch_action(Action::Type::Neutral_First);
    }

    CASE ( Walking ) //=======================================//
    {
        if (input.axis_move.y > std::abs(input.axis_move.x))
            impl_switch_action(Action::Type::Tilt_Up);

        else impl_switch_action(Action::Type::Tilt_Forward);
    }

    CASE ( Dashing, Brake ) //================================//
    {
        impl_switch_action(Action::Type::Dash_Attack);
    }

    CASE ( Crouch ) //========================================//
    {
        impl_switch_action(Action::Type::Tilt_Down);
    }

    CASE ( Landing, PreJump ) //==============================//
    {
    }

    CASE ( Jumping, Falling ) //==============================//
    {
        if (input.axis_move.x == 0.f && input.axis_move.y == 0.f)
            impl_switch_action(Action::Type::Air_Neutral);

        else if (std::abs(input.axis_move.x) >= std::abs(input.axis_move.y))
        {
            if (std::signbit(float(facing)) == std::signbit(input.axis_move.x))
                impl_switch_action(Action::Type::Air_Forward);

            else impl_switch_action(Action::Type::Air_Back);
        }

        else if (std::signbit(input.axis_move.y) == true)
            impl_switch_action(Action::Type::Air_Down);

        else impl_switch_action(Action::Type::Air_Up);
    }

    //== Not Yet Implemented =================================//

    CASE ( Attack, AirAttack, Knocked, Stunned ) {}

    //--------------------------------------------------------//

    } SWITCH_END;

    ENABLE_FLOAT_EQUALITY_WARNING;
}

//============================================================================//

void Fighter::impl_state_transition(const Transition& transition)
{
    mNextAnimation = transition.loop;
    state = transition.newState;

    play_animation(*transition.animation, transition.fadeFrames);
}

//============================================================================//

void Fighter::impl_switch_action(Action::Type actionType)
{
    SQASSERT(actionType != mActionType, "");

    if (mActiveAction != nullptr)
        mActiveAction->on_finish();

    const auto& anims = animations;

    //--------------------------------------------------------//

    SWITCH ( actionType ) {

    //--------------------------------------------------------//

    CASE ( Neutral_First )
    play_animation(anims.action_Neutral_First, 1u);
    mActiveAction = mActions->neutral_first.get();
    state = State::Attack;

    CASE ( Tilt_Down )
    play_animation(anims.action_Tilt_Down, 1u);
    mActiveAction = mActions->tilt_down.get();
    state = State::Attack;

    CASE ( Tilt_Forward )
    play_animation(anims.action_Tilt_Forward, 1u);
    mActiveAction = mActions->tilt_forward.get();
    state = State::Attack;

    CASE ( Tilt_Up )
    play_animation(anims.action_Tilt_Up, 1u);
    mActiveAction = mActions->tilt_up.get();
    state = State::Attack;

    CASE ( Air_Back )
    play_animation(anims.action_Air_Back, 1u);
    mActiveAction = mActions->air_back.get();
    state = State::AirAttack;

    CASE ( Air_Down )
    play_animation(anims.action_Air_Down, 1u);
    mActiveAction = mActions->air_down.get();
    state = State::AirAttack;

    CASE ( Air_Forward )
    play_animation(anims.action_Air_Forward, 1u);
    mActiveAction = mActions->air_forward.get();
    state = State::AirAttack;

    CASE ( Air_Neutral )
    play_animation(anims.action_Air_Neutral, 1u);
    mActiveAction = mActions->air_neutral.get();
    state = State::AirAttack;

    CASE ( Air_Up )
    play_animation(anims.action_Air_Up, 1u);
    mActiveAction = mActions->air_up.get();
    state = State::AirAttack;

    CASE ( Dash_Attack )
    play_animation(anims.action_Dash_Attack, 1u);
    mActiveAction = mActions->dash_attack.get();
    state = State::Attack;

    //--------------------------------------------------------//

    CASE ( None )
    {
        mActiveAction->on_finish();
        mActiveAction = nullptr;

        SWITCH ( mActionType ) {

        CASE ( Neutral_First, Tilt_Forward, Tilt_Up, Dash_Attack )
        impl_state_transition(transitions.attack_to_neutral);

        CASE ( Tilt_Down )
        impl_state_transition(transitions.attack_to_crouch);

        CASE ( Air_Back, Air_Down, Air_Forward, Air_Neutral, Air_Up )
        impl_state_transition(transitions.attack_to_falling);

        CASE ( None ) SQASSERT(false, "switch from None to None");

        } SWITCH_END;
    }

    //--------------------------------------------------------//

    } SWITCH_END;

    //--------------------------------------------------------//

    if (mActiveAction != nullptr)
    {
        mActiveAction->mCurrentFrame = 0u;
        mActiveAction->on_start();
    }

    mActionType = actionType;
}

//============================================================================//

void Fighter::impl_update_physics()
{
    //-- apply friction --------------------------------------//

    SWITCH ( state ) {

    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Attack, Landing )
    {
        const float friction = stats.traction * STS_LAND_FRICTION;

        if (mVelocity.x < -0.f) mVelocity.x = maths::min(mVelocity.x + friction, -0.f);
        if (mVelocity.x > +0.f) mVelocity.x = maths::max(mVelocity.x - friction, +0.f);
    }

    CASE ( Jumping, Falling, AirAttack )
    {
        const float friction = stats.air_friction * STS_AIR_FRICTION;

        if (mVelocity.x < -0.f) mVelocity.x = maths::min(mVelocity.x + friction, -0.f);
        if (mVelocity.x > +0.f) mVelocity.x = maths::max(mVelocity.x - friction, +0.f);
    }

    CASE ( PreJump )
    {
        // apply no friction
    }

    CASE ( Knocked, Stunned ) {}

    } SWITCH_END;


    //-- apply gravity ---------------------------------------//

    mVelocity.y -= stats.gravity * STS_GRAVITY;

    mVelocity.y = maths::max(mVelocity.y, stats.fall_speed * -STS_FALL_SPEED);


    //-- update position -------------------------------------//

    Stage& stage = mFightWorld.get_stage();

    const Vec2F targetPosition = current.position + mVelocity / 48.f;

    current.position = stage.transform_response(current.position, mWorldDiamond, mVelocity / 48.f);

    //sq::log_info("velocity: %s\nposition: %s\n", mVelocity, current.position);


    //-- check if fallen or landed ---------------------------//

    SWITCH ( state ) {

    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Attack, Landing )
    {
        if (current.position.y <= targetPosition.y)
        {
            //impl_state_transition(transitions.other_fall);
            play_animation(animations.falling_loop, 4u);
            state = State::Falling;
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
                impl_switch_action(Action::Type::None);

            mLandingLag = STS_LANDING_LAG;

            impl_state_transition(transitions.falling_land);
        }
    }

    CASE ( Knocked )
    {
        if (current.position.y > targetPosition.y)
        {
            if (maths::length(mVelocity) > 5.f)
            {
                // do splat stuff
            }

            mLandingLag = STS_LANDING_LAG;

            impl_state_transition(transitions.falling_land);
        }
    }

    CASE ( Stunned ) {}

    } SWITCH_END;


    //-- check if walls, ceiling, or floor reached -----------//

    if (current.position.x > targetPosition.x) mVelocity.x = 0.f;
    if (current.position.x < targetPosition.x) mVelocity.x = 0.f;
    if (current.position.y < targetPosition.y) mVelocity.y = 0.f;
    if (current.position.y > targetPosition.y) mVelocity.y = 0.f;
}

//============================================================================//

void Fighter::impl_update_active_action()
{
    if (mActiveAction != nullptr)
    {
        if (mActiveAction->on_tick(mActiveAction->mCurrentFrame))
        {
            impl_switch_action(Action::Type::None);
        }
        else ++mActiveAction->mCurrentFrame;
    }
}

//============================================================================//

void Fighter::base_tick_fighter()
{
//    constexpr auto get_state_name = [](State state)
//    {
//        switch (state)
//        {
//            case State::Neutral:   return "Neutral";
//            case State::Walking:   return "Walking";
//            case State::Dashing:   return "Dashing";
//            case State::Brake:     return "Brake";
//            case State::Crouch:    return "Crouch";
//            case State::Attack:    return "Attack";
//            case State::Landing:   return "Landing";
//            case State::PreJump:   return "PreJump";
//            case State::Jumping:   return "Jumping";
//            case State::Falling:   return "Falling";
//            case State::AirAttack: return "AirAttack";
//            case State::Knocked:   return "Knocked";
//            case State::Stunned:   return "Stunned";
//        }

//        return "";
//    };

//    if (index == 0u) sq::log_info("current state: %s", get_state_name(state));

    //--------------------------------------------------------//

    previous = current;

    //--------------------------------------------------------//

    const auto input = mController.get_input();

    impl_handle_input_movement(input);

    impl_handle_input_actions(input);

    //--------------------------------------------------------//

    impl_update_physics();

    impl_update_active_action();

    //--------------------------------------------------------//

    const Vec3F position ( current.position, 0.f );
    const QuatF rotation ( 0.f, 0.25f * float(facing), 0.f );

    mModelMatrix = maths::transform(position, rotation, Vec3F(1.f));

    //--------------------------------------------------------//

    mWorldDiamond.xNeg = mLocalDiamond.xNeg + Vec2F(mModelMatrix[3]);
    mWorldDiamond.xPos = mLocalDiamond.xPos + Vec2F(mModelMatrix[3]);
    mWorldDiamond.yNeg = mLocalDiamond.yNeg + Vec2F(mModelMatrix[3]);
    mWorldDiamond.yPos = mLocalDiamond.yPos + Vec2F(mModelMatrix[3]);
}

//============================================================================//

void Fighter::base_tick_animation()
{
    // update these animations based on horizontal velocity
    if ( mAnimation == &animations.walking_loop || mAnimation == &animations.dashing_loop )
    {
        mAnimTimeContinuous += std::abs(mVelocity.x);

        current.pose = mArmature.compute_pose(*mAnimation, mAnimTimeContinuous);
    }

    // update all other animations based on time
    else if (mAnimation != nullptr)
    {
        current.pose = mArmature.compute_pose(*mAnimation, float(mAnimTimeDiscrete));

        if (++mAnimTimeDiscrete >= int(mAnimation->totalTime))
        {
            if (mAnimation->times.back() == 0u)
            {
                mAnimation = mNextAnimation;
                mNextAnimation = nullptr;

                mAnimTimeDiscrete = 0;
                mAnimTimeContinuous = 0.f;
            }
            else mAnimTimeDiscrete = 0;
        }
    }

    // blend in old pose for basic transitions
    if (mFadeProgress != mFadeFrames)
    {
        const float blend = float(++mFadeProgress) / float(mFadeFrames + 1u);
        current.pose = mArmature.blend_poses(mFadeStartPose, current.pose, blend);
    }

    mBoneMatrices = mArmature.compute_ubo_data(current.pose);
}

//============================================================================//

void Fighter::play_animation(const Armature::Animation& animation, uint fadeFrames)
{
    mAnimation = &animation;
    mAnimTimeDiscrete = 0u;
    mAnimTimeContinuous = 0.f;

    mFadeFrames = fadeFrames;
    mFadeProgress = 0u;

    mFadeStartPose = current.pose;
}

//============================================================================//

void Fighter::apply_hit_basic(const HitBlob& hit)
{
    const float angle = maths::radians(hit.knockAngle * float(hit.fighter->facing));
    const Vec2F knockDir = { std::sin(angle), std::cos(angle) };

    mVelocity = maths::normalize(knockDir) * hit.knockBase;

    //play_animation(anims.knocked, 0u);
    state = State::Knocked;
}

//============================================================================//

Mat4F Fighter::interpolate_model_matrix(float blend) const
{
    const Vec2F position = maths::mix(previous.position, current.position, blend);
    const QuatF rotation = QuatF(0.f, 0.25f * float(facing), 0.f);

    return maths::transform(Vec3F(position, 0.f), rotation, Vec3F(1.f));
}

std::vector<Mat34F> Fighter::interpolate_bone_matrices(float blend) const
{
    auto blendPose = mArmature.blend_poses(previous.pose, current.pose, blend);
    return mArmature.compute_ubo_data(blendPose);
}
