#include "game/private/PrivateFighter.hpp"

#include "game/Stage.hpp"

#include <sqee/macros.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

namespace maths = sq::maths;

using sq::literals::operator""_fmt_;

using namespace sts;

//============================================================================//

static constexpr float STS_WALK_SPEED           = 3.0f;  // walk_speed
static constexpr float STS_DASH_SPEED           = 6.0f;  // dash_speed
static constexpr float STS_AIR_SPEED            = 3.0f;  // air_speed

static constexpr float STS_WALK_MOBILITY        = 1.0f;  // traction
static constexpr float STS_DASH_MOBILITY        = 1.5f;  // traction
static constexpr float STS_LAND_FRICTION        = 0.5f;  // traction

static constexpr float STS_AIR_MOBILITY         = 0.5f;  // air_mobility
static constexpr float STS_AIR_FRICTION         = 0.2f;  // air_friction

static constexpr float STS_HOP_HEIGHT           = 2.0f;  // hop_height
static constexpr float STS_JUMP_HEIGHT          = 3.0f;  // jump_height
static constexpr float STS_AIR_HOP_HEIGHT       = 2.0f;  // air_hop_height

static constexpr float STS_GRAVITY              = 0.5f;  // gravity
static constexpr float STS_FALL_SPEED           = 12.0f; // fall_speed

static constexpr float STS_TICK_RATE            = 48.0f;
static constexpr uint  STS_LANDING_LAG          = 4u;
static constexpr uint  STS_JUMP_DELAY           = 4u;
static constexpr uint  STS_DASH_TURN_LIMIT      = 4u;

static constexpr float STS_LEDGE_DROP_VELOCITY  = 1.0f;
static constexpr uint  STS_NO_LEDGE_CATCH_TIME  = 48u;

//============================================================================//

void PrivateFighter::initialise_armature(const String& path)
{
    armature.load_bones(path + "/Bones.txt", true);
    armature.load_rest_pose(path + "/RestPose.txt");

    current.pose = previous.pose = armature.get_rest_pose();

    fighter.mBoneMatrices.resize(armature.get_bone_count());

    //--------------------------------------------------------//

    constexpr const auto DEFAULT_ANIMATIONS = std::array
    {
        std::tuple { "DashingLoop", AnimMode::DashCycle },
        std::tuple { "FallingLoop", AnimMode::Standard },
        std::tuple { "NeutralLoop", AnimMode::Standard },
        std::tuple { "WalkingLoop", AnimMode::WalkCycle },

        std::tuple { "VertigoStart", AnimMode::Standard },
        std::tuple { "VertigoLoop",  AnimMode::Standard },

        std::tuple { "ShieldOn",   AnimMode::Standard }, // GuardOn
        std::tuple { "ShieldOff",  AnimMode::Standard }, // GuradOff
        std::tuple { "ShieldLoop", AnimMode::Standard }, // Guard

        std::tuple { "CrouchOn",   AnimMode::Standard }, // Squat
        std::tuple { "CrouchOff",  AnimMode::Standard }, // SquatRv
        std::tuple { "CrouchLoop", AnimMode::Standard }, // SquatWait

        std::tuple { "Brake",     AnimMode::BrakeSlow }, // finish dashing
        std::tuple { "DiveDash",  AnimMode::Standard }, // dash off an edge
        std::tuple { "DiveWalk",  AnimMode::Standard }, // walk off an edge
        std::tuple { "Dodge",     AnimMode::Standard }, // dodge on the ground
        std::tuple { "LandClean", AnimMode::Standard }, // land normally
        std::tuple { "PreJump",   AnimMode::Standard }, // JumpSquat
        std::tuple { "Turn",      AnimMode::Standard }, // turn around while standing
        std::tuple { "TurnBrake", AnimMode::BrakeSlow }, // brake after turning after dashing
        std::tuple { "TurnDash",  AnimMode::BrakeSlow }, // turn and dash after braking

        std::tuple { "EvadeBack",    AnimMode::ApplyMotion }, // evade backwards
        std::tuple { "EvadeForward", AnimMode::ApplyMotion }, // evade forwards
        std::tuple { "Dodge",        AnimMode::Standard },    // dodge on the spot
        std::tuple { "AirDodge",     AnimMode::Standard },    // dodge in the air

        std::tuple { "JumpBack",    AnimMode::JumpAscend }, // begin a back jump
        std::tuple { "JumpForward", AnimMode::JumpAscend }, // begin a forward jump
        std::tuple { "AirHop",      AnimMode::JumpAscend }, // begin an extra jump

        std::tuple { "LedgeCatch", AnimMode::Standard }, // CliffCatch
        std::tuple { "LedgeLoop",  AnimMode::Standard }, // CliffWait
        std::tuple { "LedgeClimb", AnimMode::FixedMotion }, // CliffClimbQuick
        std::tuple { "LedgeJump",  AnimMode::Standard }, // CliffJumpQuick

        std::tuple { "Knocked", AnimMode::Standard },

        std::tuple { "NeutralFirst", AnimMode::Standard },

        std::tuple { "TiltDown",    AnimMode::Standard },
        std::tuple { "TiltForward", AnimMode::Standard },
        std::tuple { "TiltUp",      AnimMode::Standard },

        std::tuple { "AirBack",    AnimMode::Standard },
        std::tuple { "AirDown",    AnimMode::Standard },
        std::tuple { "AirForward", AnimMode::Standard },
        std::tuple { "AirNeutral", AnimMode::Standard },
        std::tuple { "AirUp",      AnimMode::Standard },

        std::tuple { "DashAttack", AnimMode::Standard },

        std::tuple { "SmashDownStart",    AnimMode::Standard },
        std::tuple { "SmashForwardStart", AnimMode::Standard },
        std::tuple { "SmashUpStart",      AnimMode::Standard },

        std::tuple { "SmashDownCharge",    AnimMode::Standard },
        std::tuple { "SmashForwardCharge", AnimMode::Standard },
        std::tuple { "SmashUpCharge",      AnimMode::Standard },

        std::tuple { "SmashDownAttack",    AnimMode::Standard },
        std::tuple { "SmashForwardAttack", AnimMode::Standard },
        std::tuple { "SmashUpAttack",      AnimMode::Standard },

        std::tuple { "Null", AnimMode::Manual }
    };

    for (const auto& [key, mode] : DEFAULT_ANIMATIONS)
    {
        const String filePath = sq::build_path(path, "anims", sq::to_c_string(key)) + ".txt";
        if (sq::check_file_exists(filePath) == false)
        {
            sq::log_warning("missing animation '%s'", filePath);
            fighter.animations[key] = { armature.make_animation(path + "/anims/Null.txt"), mode };
            continue;
        }
        fighter.animations[key] = { armature.make_animation(filePath), mode };
    }

    //--------------------------------------------------------//

    const auto a = [&](const char* key) { return &fighter.animations.at(key); };

    Transitions& t = transitions;

    t.neutral_crouch  = { State::Crouch,  2u, a("CrouchOn"), a("CrouchLoop") };
    t.neutral_shield  = { State::Shield,  2u, a("ShieldOn"), a("ShieldLoop") };
    t.neutral_walking = { State::Walking, 4u, a("WalkingLoop"), nullptr };

    t.walking_crouch  = { State::Crouch,  2u, a("CrouchOn"), a("CrouchLoop") };
    t.walking_shield  = { State::Shield,  2u, a("ShieldOn"), a("ShieldLoop") };
    t.walking_dashing = { State::Dashing, 4u, a("DashingLoop"), nullptr };
    t.walking_dive    = { State::Falling, 2u, a("DiveWalk"), a("FallingLoop") };
    t.walking_neutral = { State::Neutral, 4u, a("NeutralLoop"), nullptr };

    t.dashing_shield = { State::Shield,  2u, a("ShieldOn"), a("ShieldLoop") };
    t.dashing_brake  = { State::Brake,   4u, a("Brake"), a("NeutralLoop") };
    t.dashing_dive   = { State::Falling, 2u, a("DiveDash"), a("FallingLoop") };

    t.crouch_shield  = { State::Shield,  2u, a("ShieldLoop"), nullptr };
    t.crouch_neutral = { State::Neutral, 2u, a("CrouchOff"), a("NeutralLoop") };

    t.air_dodge = { State::AirDodge,  1u, a("AirDodge"), a("FallingLoop") };
    t.air_hop   = { State::Jumping,   1u, a("AirHop"), a("FallingLoop") };
    t.air_ledge = { State::LedgeHang, 1u, a("LedgeCatch"), a("LedgeLoop") };

    t.jumping_falling = { State::Falling,  8u, a("FallingLoop"), nullptr };

    t.shield_dodge         = { State::Dodge,        1u, a("Dodge"), a("NeutralLoop") };
    t.shield_evade_back    = { State::EvadeBack,    1u, a("EvadeBack"), nullptr };
    t.shield_evade_forward = { State::EvadeForward, 1u, a("EvadeForward"), nullptr };
    t.shield_neutral       = { State::Neutral,      1u, a("ShieldOff"), a("NeutralLoop") };

    t.ledge_climb = { State::LedgeClimb, 1u, a("LedgeClimb"), a("NeutralLoop") };
    t.ledge_jump  = { State::Jumping,    1u, a("LedgeJump"), a("FallingLoop") };
    t.ledge_drop  = { State::Falling,    0u, a("FallingLoop"), nullptr };

    t.misc_prejump = { State::PreJump, 1u, a("PreJump"), nullptr };
    t.jump_back    = { State::Jumping, 1u, a("JumpBack"), a("FallingLoop") };
    t.jump_forward = { State::Jumping, 1u, a("JumpForward"), a("FallingLoop") };
    t.land_clean   = { State::Landing, 1u, a("LandClean"), a("NeutralLoop") };

    t.neutral_turn = { State::Neutral, 1u, a("Turn"), a("NeutralLoop") };

    t.brake_turn_dash = { State::Brake, 2u, a("TurnDash"), nullptr };
    t.brake_turn_brake = { State::Brake, 0u, a("TurnBrake"), nullptr };

    t.instant_crouch  = { State::Crouch,  0u, a("CrouchLoop"), nullptr };
    t.instant_falling = { State::Falling, 0u, a("FallingLoop"), nullptr };
    t.instant_neutral = { State::Neutral, 0u, a("NeutralLoop"), nullptr };
    t.instant_shield  = { State::Shield,  0u, a("ShieldLoop"), nullptr };
    t.instant_dashing = { State::Dashing, 0u, a("DashingLoop"), nullptr };

    t.misc_vertigo = { State::Neutral, 2u, a("VertigoStart"), a("VertigoLoop") };

    t.neutral_attack = { State::Attack, 1u, a("NeutralFirst"), nullptr };

    t.tilt_down_attack    = { State::Attack, 1u, a("TiltDown"), nullptr };
    t.tilt_forward_attack = { State::Attack, 1u, a("TiltForward"), nullptr };
    t.tilt_up_attack      = { State::Attack, 1u, a("TiltUp"), nullptr };

    t.air_back_attack    = { State::AirAttack, 1u, a("AirBack"), nullptr };
    t.air_down_attack    = { State::AirAttack, 1u, a("AirDown"), nullptr };
    t.air_forward_attack = { State::AirAttack, 1u, a("AirForward"), nullptr };
    t.air_neutral_attack = { State::AirAttack, 1u, a("AirNeutral"), nullptr };
    t.air_up_attack      = { State::AirAttack, 1u, a("AirUp"), nullptr };

    t.dash_attack = { State::Attack, 1u, a("DashAttack"), nullptr };

    t.smash_down_start    = { State::Charge, 1u, a("SmashDownStart"), a("SmashDownCharge") };
    t.smash_forward_start = { State::Charge, 1u, a("SmashForwardStart"), a("SmashForwardCharge") };
    t.smash_up_start      = { State::Charge, 1u, a("SmashUpStart"), a("SmashUpCharge") };

    t.smash_down_attack    = { State::Attack, 1u, a("SmashDownAttack"), nullptr };
    t.smash_forward_attack = { State::Attack, 1u, a("SmashForwardAttack"), nullptr };
    t.smash_up_attack      = { State::Attack, 1u, a("SmashUpAttack"), nullptr };

    t.editor_preview = { State::EditorPreview, 0u, a("Null"), nullptr };
}

//============================================================================//

void PrivateFighter::initialise_hurt_blobs(const String& path)
{
    const JsonValue root = sq::parse_json_from_file(path + "/HurtBlobs.json");
    for (const auto& item : root.items())
    {
        HurtBlob& blob = fighter.hurtBlobs[item.key()];

        blob.fighter = &fighter;

        try { blob.from_json(item.value()); }
        catch (const std::exception& e) {
            sq::log_warning("problem loading hurt blob '%s': %s", item.key(), e.what());
        }

        get_world().enable_hurt_blob(&blob);
    }
}

//============================================================================//

void PrivateFighter::initialise_stats(const String& path)
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

    stats.dodge_finish     = json.at("dodge_finish");
    stats.dodge_safe_start = json.at("dodge_safe_start");
    stats.dodge_safe_end   = json.at("dodge_safe_end");

    stats.evade_back_finish     = json.at("evade_back_finish");
    stats.evade_back_safe_start = json.at("evade_back_safe_start");
    stats.evade_back_safe_end   = json.at("evade_back_safe_end");

    stats.evade_forward_finish     = json.at("evade_forward_finish");
    stats.evade_forward_safe_start = json.at("evade_forward_safe_start");
    stats.evade_forward_safe_end   = json.at("evade_forward_safe_end");

    stats.air_dodge_finish     = json.at("air_dodge_finish");
    stats.air_dodge_safe_start = json.at("air_dodge_safe_start");
    stats.air_dodge_safe_end   = json.at("air_dodge_safe_end");

    stats.ledge_climb_finish = json.at("ledge_climb_finish");

    stats.anim_walk_stride = json.at("anim_walk_stride");
    stats.anim_dash_stride = json.at("anim_dash_stride");

    //stats.anim_evade_distance       = json.at("anim_evade_distance");
    //stats.anim_ledge_climb_distance = json.at("anim_ledge_climb_distance");

    //SQASSERT(stats.dodge_finish == fighter.animations.at("Dodge").anim.totalTime, "");
    SQASSERT(stats.dodge_safe_start < stats.dodge_safe_end, "");
    SQASSERT(stats.dodge_safe_end < stats.dodge_finish, "");

    //SQASSERT(stats.evade_back_finish == fighter.animations.at("EvadeBack").anim.totalTime, "");
    SQASSERT(stats.evade_back_safe_start < stats.evade_back_safe_end, "");
    SQASSERT(stats.evade_back_safe_end < stats.evade_back_finish, "");

    //SQASSERT(stats.evade_forward_finish == fighter.animations.at("EvadeForward").anim.totalTime, "");
    SQASSERT(stats.evade_forward_safe_start < stats.evade_forward_safe_end, "");
    SQASSERT(stats.evade_forward_safe_end < stats.evade_forward_finish, "");

    //SQASSERT(stats.air_dodge_finish == fighter.animations.at("AirDodge").anim.totalTime, "");
    SQASSERT(stats.air_dodge_safe_start < stats.air_dodge_safe_end, "");
    SQASSERT(stats.air_dodge_safe_end < stats.air_dodge_finish, "");
}

//============================================================================//

void PrivateFighter::initialise_actions(const String& path)
{
    for (int8_t i = 0; i < sq::enum_count_v<ActionType>; ++i)
    {
        const auto actionType = ActionType(i);
        const auto actionPath = sq::build_string(path, "/actions/", sq::to_c_string(actionType), ".json");

        fighter.actions[i] = std::make_unique<Action>(get_world(), fighter, actionType, actionPath);
        get_world().get_action_builder().load_from_json(*fighter.actions[i]);
    }
}

//============================================================================//

void PrivateFighter::handle_input_movement(const Controller::Input& input)
{
    const Stats& stats = fighter.stats;

    State& state = fighter.current.state;
    Facing& facing = fighter.current.facing;

    Vec2F& velocity = fighter.mVelocity;
    Vec2F& translate = fighter.mTranslate;

    Status& status = fighter.status;

    Stage& stage = get_world().get_stage();

    //--------------------------------------------------------//

    mMoveAxisX = input.int_axis.x;
    mMoveAxisY = input.int_axis.y;

    //-- catching ledges takes priority, so we do it first ---//

    if (status.timeSinceLedge > STS_NO_LEDGE_CATCH_TIME)
    {
        if (state == State::Jumping || state == State::Falling)
        {
            // todo: unsure if position should be fighter's origin, or the centre of it's diamond
            status.ledge = stage.find_ledge(current.position, mMoveAxisX);
            if (status.ledge != nullptr)
            {
                // steal the ledge from some other fighter
                if (status.ledge->grabber != nullptr)
                    status.ledge->grabber->status.ledge = nullptr;

                status.ledge->grabber = &fighter;
                facing = Facing(-status.ledge->direction);
            }
        }
    }

    //-- apply other pre-transition updates ------------------//

    if (input.press_jump == true)
    {
        const auto start_jump = [&]()
        {
            mJumpHeld = true;
        };

        const auto start_air_hop = [&]()
        {
            mJumpHeld = false;

            const float height = stats.air_hop_height * STS_AIR_HOP_HEIGHT;
            const float gravity = stats.gravity * STS_GRAVITY;

            mJumpVelocity = std::sqrt(2.f * gravity * height * STS_TICK_RATE);
            velocity.y = mJumpVelocity + gravity * 0.5f;
        };

        SWITCH (state) {
            CASE ( Walking, Dashing ) start_jump(); // moving jump
            CASE ( Neutral, Crouch, Brake, Shield ) start_jump(); // standing jump
            CASE ( Jumping, Falling, LedgeHang ) start_air_hop(); // aerial jump
            CASE_DEFAULT {} // no jump allowed
        } SWITCH_END;
    }

    // if we are about to start moving, then face that direction
    else if (state == State::Neutral)
    {
        if (input.float_axis.x < -0.f) facing = Facing::Left;
        if (input.float_axis.x > +0.f) facing = Facing::Right;
    }

    // we can drop from ledge by pressing either down or back
    else if (state == State::LedgeHang)
    {
        if ( status.ledge == nullptr || // someone else stole the ledge
             (input.int_axis.y < +2 && (input.int_axis.y == -2 || input.int_axis.x / 2 == status.ledge->direction)) )
        {
            translate.x = (current.pose[0].offset.z - fighter.diamond.halfWidth) * float(facing);
            translate.y = current.pose[0].offset.y;
            velocity.x = STS_LEDGE_DROP_VELOCITY * -float(facing);
            status.timeSinceLedge = 0u;
        }
    }

    // we can turn around while braking, then dash in the other direction
    else if (state == State::Brake)
    {
        if (mDoBrakeTurn == 0)
        {
            if (input.int_axis.x == -int8_t(facing))
            {
                mDoBrakeTurn = -int8_t(facing);
                state_transition(transitions.brake_turn_dash);
            }
        }

        else if (mDoTurnDash == false && Facing(mDoBrakeTurn) != facing)
        {
            if (mStateProgress < STS_DASH_TURN_LIMIT)
            {
                if (input.mash_axis.x == mDoBrakeTurn)
                    mDoTurnDash = true;
            }

            else if (mStateProgress == STS_DASH_TURN_LIMIT)
            {
                facing = Facing(mDoBrakeTurn);
                mBrakeVelocity = std::abs(velocity.x);
                state_transition(transitions.brake_turn_brake);
            }
        }
    }

    //-- transitions in response to input --------------------//

    DISABLE_WARNING_FLOAT_EQUALITY;

    SWITCH ( state ) {

    CASE ( Neutral ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_prejump);

        else if (input.hold_shield == true)
            state_transition(transitions.neutral_shield);

        else if (input.float_axis.y == -1.f)
            state_transition(transitions.neutral_crouch);

        else if (input.float_axis.x != 0.f)
            state_transition(transitions.neutral_walking);
    }

    CASE ( Walking ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_prejump);

        else if (input.press_shield == true)
            state_transition(transitions.walking_shield);

        else if (input.float_axis.y == -1.f)
            state_transition(transitions.walking_crouch);

        else if (input.float_axis.x == 0.f)
            state_transition(transitions.walking_neutral);

        else if (input.mash_axis.x != 0)
            state_transition(transitions.walking_dashing);
    }

    CASE ( Dashing ) //=======================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_prejump);

        else if (input.press_shield == true)
            state_transition(transitions.dashing_shield);

        else if (input.norm_axis.x != int8_t(facing))
        {
            mBrakeVelocity = std::abs(velocity.x);
            state_transition(transitions.dashing_brake);
        }
    }

    CASE ( Brake ) //=========================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_prejump);

        //else if (input.int_axis.x == int8_t(facing) * -1)
        //    state_transition(transitions.brake_turn_brake);

        //else if (input.mash_axis.x == -int8_t(facing))
        //    state_transition(transitions.brake_turn_dash);
    }

    CASE ( Crouch ) //========================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_prejump);

        if (input.press_shield == true)
            state_transition(transitions.crouch_shield);

        else if (input.float_axis.y != -1.f)
            state_transition(transitions.crouch_neutral);
    }

    CASE ( PreJump ) //=======================================//
    {
        mJumpHeld &= input.hold_jump;
    }

    CASE ( Jumping ) //=======================================//
    {
        mJumpHeld &= input.hold_jump;

        if (status.ledge != nullptr)
            state_transition(transitions.air_ledge);

        else if (input.press_jump == true)
            state_transition(transitions.air_hop);

        else if (input.press_shield == true)
            state_transition(transitions.air_dodge);
    }

    CASE ( Falling ) //=======================================//
    {
        if (status.ledge != nullptr)
            state_transition(transitions.air_ledge);

        else if (input.press_jump == true)
            state_transition(transitions.air_hop);

        else if (input.press_shield == true)
            state_transition(transitions.air_dodge);
    }

    CASE ( Shield ) //========================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.misc_prejump);

        else if (input.hold_shield == false)
            state_transition(transitions.shield_neutral);

        else if (input.mash_axis.x == -int8_t(facing))
            state_transition(transitions.shield_evade_back);

        else if (input.mash_axis.x == +int8_t(facing))
            state_transition(transitions.shield_evade_forward);

        else if (input.mash_axis.y != 0)
            state_transition(transitions.shield_dodge);
    }

    CASE ( LedgeHang ) //=====================================//
    {
        if (input.press_jump == true)
            state_transition(transitions.ledge_jump);

        else if (input.int_axis.y == +2)
            state_transition(transitions.ledge_climb);

        else if (status.ledge == nullptr)
            state_transition(transitions.ledge_drop);

        else if (input.int_axis.y == -2 || input.int_axis.x / 2 == status.ledge->direction)
            state_transition(transitions.ledge_drop);

    }

    //== Nothing to do here ==================================//

    CASE ( Attack, AirAttack, Special, AirSpecial ) {}
    CASE ( Landing, Charge, Dodge, EvadeBack, EvadeForward, AirDodge ) {}
    CASE ( LedgeClimb ) {}

    CASE ( EditorPreview ) {}

    //== Not Yet Implemented =================================//

    CASE ( Knocked, Stunned ) {}

    //--------------------------------------------------------//

    } SWITCH_END;

    ENABLE_WARNING_FLOAT_EQUALITY;

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

    //-- clear ungrabbed ledge -------------------------------//

    if (state != State::LedgeHang && status.ledge != nullptr)
    {
        status.ledge->grabber = nullptr;
        status.ledge = nullptr;
    }
}

//============================================================================//

void PrivateFighter::handle_input_actions(const Controller::Input& input)
{
    const State state = fighter.current.state;
    const Facing facing = fighter.current.facing;
    Action* const action = fighter.current.action;

    //--------------------------------------------------------//

    DISABLE_WARNING_FLOAT_EQUALITY;

    SWITCH ( state ) {

    //--------------------------------------------------------//

    CASE ( Neutral ) //=======================================//
    {
        if (input.press_attack == false) return;

        if      (input.mod_axis.y == -1) switch_action(ActionType::Smash_Down);
        else if (input.mod_axis.y == +1) switch_action(ActionType::Smash_Up);

        else if (input.float_axis.y < -0.f) switch_action(ActionType::Tilt_Down);
        else if (input.float_axis.y > +0.f) switch_action(ActionType::Tilt_Up);

        else switch_action(ActionType::Neutral_First);
    }

    CASE ( Walking ) //=======================================//
    {
        if (input.press_attack == false) return;

        // todo: work out correct priority here

        if (input.mod_axis.x != 0)
            switch_action(ActionType::Smash_Forward);

        else if (input.float_axis.y > std::abs(input.float_axis.x))
            switch_action(ActionType::Tilt_Up);

        else switch_action(ActionType::Tilt_Forward);
    }

    CASE ( Dashing ) //=======================================//
    {
        if (input.press_attack == false) return;

        if (input.mod_axis.x != 0) switch_action(ActionType::Smash_Forward);

        else switch_action(ActionType::Dash_Attack);
    }

    CASE ( Brake ) //=========================================//
    {
        if (input.press_attack == false) return;

        if (input.mod_axis.y == +1) switch_action(ActionType::Smash_Up);

        else switch_action(ActionType::Dash_Attack);
    }

    CASE ( Crouch ) //========================================//
    {
        if (input.press_attack == false) return;

        if (input.mod_axis.y == -1) switch_action(ActionType::Smash_Down);

        else switch_action(ActionType::Tilt_Down);
    }

    CASE ( Jumping, Falling ) //==============================//
    {
        if (input.press_attack == false) return;

        // todo: work out correct priority here

        if (input.float_axis.x == 0.f && input.float_axis.y == 0.f)
            switch_action(ActionType::Air_Neutral);

        else if (std::abs(input.float_axis.x) >= std::abs(input.float_axis.y))
        {
            if (std::signbit(float(facing)) == std::signbit(input.float_axis.x))
                switch_action(ActionType::Air_Forward);

            else switch_action(ActionType::Air_Back);
        }

        else if (std::signbit(input.float_axis.y) == true)
            switch_action(ActionType::Air_Down);

        else switch_action(ActionType::Air_Up);
    }

    CASE ( Charge ) //========================================//
    {
        if (input.hold_attack == false) // todo: or max charge reached
        {
            if (action && action->get_type() == ActionType::Smash_Down)
                state_transition(transitions.smash_down_attack);

            else if (action && action->get_type() == ActionType::Smash_Forward)
                state_transition(transitions.smash_forward_attack);

            else if (action && action->get_type() == ActionType::Smash_Up)
                state_transition(transitions.smash_up_attack);

            else SQASSERT(false, "invalid action for charge");

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

    CASE ( LedgeHang ) //=====================================//
    {
        // ledge attacks go here
    }

    //== Nothing to do here ==================================//

    CASE ( PreJump, Landing, Knocked, Stunned ) {}
    CASE ( Shield, Dodge, EvadeBack, EvadeForward, AirDodge ) {}
    CASE ( LedgeClimb ) {}

    CASE ( EditorPreview ) {}

    //--------------------------------------------------------//

    } SWITCH_END;

    ENABLE_WARNING_FLOAT_EQUALITY;
}

//============================================================================//

void PrivateFighter::state_transition(const Transition& transition, bool keepTime)
{
    fighter.current.state = transition.newState;

    if (keepTime == false)
    {
        mAnimTimeDiscrete = 0u;
        mAnimTimeContinuous = 0.f;
    }

    mFadeProgress = 0u;
    mStateProgress = 0u;

    mVertigoActive = false;

    mAnimation = transition.animation;
    mNextAnimation = transition.loop;
    mFadeFrames = transition.fadeFrames;

    if (get_world().globals.editorMode == true)
        mFadeFrames = 0u;

    mStaticPose = nullptr;

    mFadeStartPose = current.pose;
}

//============================================================================//

void PrivateFighter::switch_action(ActionType type)
{
    Action* const newAction = fighter.get_action(type);

    SQASSERT(newAction != fighter.current.action, "switch to same action");

    //--------------------------------------------------------//

    SWITCH ( type ) {

    //--------------------------------------------------------//

    CASE ( Neutral_First ) state_transition(transitions.neutral_attack);

    CASE ( Tilt_Down )    state_transition(transitions.tilt_down_attack);
    CASE ( Tilt_Forward ) state_transition(transitions.tilt_forward_attack);
    CASE ( Tilt_Up )      state_transition(transitions.tilt_up_attack);

    CASE ( Air_Back )    state_transition(transitions.air_back_attack);
    CASE ( Air_Down )    state_transition(transitions.air_down_attack);
    CASE ( Air_Forward ) state_transition(transitions.air_forward_attack);
    CASE ( Air_Neutral ) state_transition(transitions.air_neutral_attack);
    CASE ( Air_Up )      state_transition(transitions.air_up_attack);

    CASE ( Dash_Attack ) state_transition(transitions.dash_attack);

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

        SWITCH ( fighter.current.action->get_type() ) {

        CASE ( Neutral_First, Tilt_Forward, Tilt_Up, Smash_Down, Smash_Forward, Smash_Up, Dash_Attack )
        state_transition(transitions.instant_neutral);

        CASE ( Tilt_Down )
        state_transition(transitions.instant_crouch);

        CASE ( Air_Back, Air_Down, Air_Forward, Air_Neutral, Air_Up )
        state_transition(transitions.instant_falling);

        CASE ( Special_Down, Special_Forward, Special_Neutral, Special_Up )
        state_transition(transitions.instant_neutral); // todo

        CASE ( None ) SQASSERT(false, "switch from None to None");

        } SWITCH_END;
    }

    //--------------------------------------------------------//

    } SWITCH_END;

    //--------------------------------------------------------//

    if (fighter.current.action) fighter.current.action->do_finish();

    fighter.current.action = newAction;

    if (fighter.current.action) fighter.current.action->do_start();

}

//============================================================================//

void PrivateFighter::update_after_input()
{
    const Stats& stats = fighter.stats;

    Stage& stage = get_world().get_stage();

    State& state = fighter.current.state;
    Facing& facing = fighter.current.facing;

    Vec2F& velocity = fighter.mVelocity;
    Vec2F& translate = fighter.mTranslate;

    Status& status = fighter.status;

    mStateProgress += 1u;

    //-- most updates don't apply when ledge hanging ---------//

    if (state == State::LedgeHang)
    {
        SQASSERT(status.ledge != nullptr, "nope");

        // this will be the case only if we just grabbed the ledge
        if (velocity != Vec2F())
        {
            current.position = (current.position + status.ledge->position) / 2.f;
            velocity = Vec2F();
        }
        else current.position = status.ledge->position;

        return; // EARLY RETURN
    }

    //--------------------------------------------------------//

    Vec2F targetPosition = current.position;

    status.timeSinceLedge += 1u;

    //-- apply friction --------------------------------------//

    SWITCH ( state ) {

    CASE ( Neutral, Walking, Dashing, Brake, Crouch, Charge, Attack, Special, Landing, Shield, Dodge, EvadeBack, EvadeForward )
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

    CASE ( PreJump, Knocked, Stunned, LedgeHang, LedgeClimb ) {}

    CASE ( EditorPreview ) {}

    } SWITCH_END;

    //-- perform passive state updates -----------------------//

    constexpr auto in_range = [](uint value, uint min, uint max) { return value >= min && value < max; };

    if (state == State::Dodge)
    {
        status.intangible = in_range(mStateProgress, stats.dodge_safe_start, stats.dodge_safe_end);

        if (mStateProgress > stats.dodge_finish)
            state_transition(transitions.instant_neutral);
    }

    else if (state == State::EvadeBack)
    {
        status.intangible = in_range(mStateProgress, stats.evade_back_safe_start, stats.evade_back_safe_end);

        if (mStateProgress > stats.evade_back_finish)
            state_transition(transitions.instant_neutral);
    }

    else if (state == State::EvadeForward)
    {
        status.intangible = in_range(mStateProgress, stats.evade_forward_safe_start, stats.evade_forward_safe_end);

        if (mStateProgress > stats.evade_forward_finish)
        {
            facing = Facing(-int8_t(facing));
            state_transition(transitions.instant_neutral);
        }
    }

    else if (state == State::AirDodge)
    {
        status.intangible = in_range(mStateProgress, stats.air_dodge_safe_start, stats.air_dodge_safe_end);

        // change to Falling instead of neutral to disable L-canceling
        if (mStateProgress > stats.air_dodge_finish)
            state_transition(transitions.instant_neutral);
    }

    else if (state == State::LedgeClimb)
    {
        if (mStateProgress > stats.ledge_climb_finish)
            state_transition(transitions.instant_neutral);
    }

    else if (state == State::Landing)
    {
        // no transition, neutral loop will play after landing animation
        if (mStateProgress == STS_LANDING_LAG)
            state = State::Neutral;
    }

    else if (state == State::PreJump)
    {
        // todo: move as much of this as possible into handle_input_movement
        if (mStateProgress == STS_JUMP_DELAY)
        {
            if (mMoveAxisX == -int8_t(facing)) state_transition(transitions.jump_back);
            else if (mMoveAxisX == -int8_t(facing) * 2) state_transition(transitions.jump_back);
            else state_transition(transitions.jump_forward);

            const float hopHeight = stats.hop_height * STS_HOP_HEIGHT;
            const float jumpHeight = stats.jump_height * STS_JUMP_HEIGHT;
            const float gravity = stats.gravity * STS_GRAVITY;

            mJumpVelocity = std::sqrt(2.f * (mJumpHeld ? jumpHeight : hopHeight) * gravity * STS_TICK_RATE);

            velocity.y = mJumpVelocity;
        }
    }

    else if (state == State::Jumping)
    {
        // no transition, falling loop will play after jumping animation
        if (velocity.y <= 0.f)
            state = State::Falling;
    }

    else if (state == State::Brake)
    {
        if (velocity.x == 0.f)
        {
            if (mDoTurnDash == false)
            {
                mDoBrakeTurn = 0;
                state_transition(transitions.instant_neutral);
            }
            else
            {
                facing = Facing(mDoBrakeTurn);
                velocity.x = float(facing) * stats.traction * STS_DASH_MOBILITY * 2.f;
                mDoBrakeTurn = 0;
                mDoTurnDash = false;
                state_transition(transitions.instant_dashing);
            }
        }
    }

    else if (state == State::Knocked)
    {

    }

    //-- apply gravity ---------------------------------------//

    velocity.y -= stats.gravity * STS_GRAVITY;

    velocity.y = maths::max(velocity.y, stats.fall_speed * -STS_FALL_SPEED);

    //-- update position -------------------------------------//

    targetPosition += velocity / STS_TICK_RATE + translate;
    translate = Vec2F();

    const bool edgeStop = ( state == State::EvadeBack || state == State::EvadeForward || state == State::Attack ||
                            state == State::Special || state == State::Shield ) ||
                          ( (state == State::Walking || state == State::Brake || state == State::Neutral) &&
                            std::abs(mMoveAxisX) < 2 );

    MoveAttempt moveAttempt = stage.attempt_move(fighter.diamond, current.position, targetPosition, edgeStop);

    if ( moveAttempt.edge == int8_t(facing) && mVertigoActive == false &&
         (state == State::Walking || state == State::Brake || state == State::Neutral) )
    {
        // vertigo is just neutral with a different animation
        state_transition(transitions.misc_vertigo);
        mVertigoActive = true;
    }

    current.position = moveAttempt.result;

    //-- check if fallen or moved off an edge ----------------//

    if (moveAttempt.collideFloor == false)
    {
        SWITCH (state) {
            CASE (Walking) { state_transition(transitions.walking_dive); }
            CASE (Dashing) { state_transition(transitions.dashing_dive); }
            CASE (Special) {} // todo
            CASE (PreJump, Jumping, Falling, AirAttack, AirSpecial, AirDodge, Knocked, LedgeHang) {}
            CASE_DEFAULT { state_transition(transitions.instant_falling); }
        } SWITCH_END;
    }

    //-- check if landed on the ground -----------------------//

    if (moveAttempt.collideFloor == true)
    {
        // being on solid floor always clears velocity
        velocity.y = 0.f;

        if (state == State::AirDodge || state == State::Jumping || state == State::Falling)
        {
            state_transition(transitions.land_clean);
        }

        else if (state == State::AirAttack)
        {
            state_transition(transitions.land_clean);
            switch_action(ActionType::None);
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
            state_transition(transitions.land_clean);
        }
    }

    //-- check if we've hit our head on the ceiling ----------//

    if (moveAttempt.collideCeiling == true)
    {
        // todo: not sure how to handle this, maybe we should keep velocity to
        // make sliding up from under the stage easier, will need to test
        velocity.y = 0.f;
    }

    //-- check if moving into a wall -------------------------//

    if (moveAttempt.collideWall == true)
    {
        if (state == State::Walking || state == State::Dashing)
            state_transition(transitions.instant_neutral);

        // todo: we may want to keep velocity in the air, will need to test
        velocity.x = 0.f;
    }

    //-- update the active action ----------------------------//

    if (fighter.current.action != nullptr && fighter.current.state != State::Charge)
    {
        if (fighter.current.action->do_tick() == true)
        {
            message::fighter_action_finished msg { fighter, fighter.current.action->get_type() };
            switch_action(ActionType::None);

            get_world().get_message_bus().get_message_source<message::fighter_action_finished>().publish(msg);
        }
    }
}

//============================================================================//

void PrivateFighter::base_tick_fighter()
{
    fighter.previous = fighter.current;

    previous = current;

    //--------------------------------------------------------//

    if (get_world().globals.editorMode == true)
    {
        if (fighter.current.state != State::EditorPreview)
        {
            const auto input = Controller::Input();

            handle_input_movement(input);
            handle_input_actions(input);
        }
    }
    else
    {
        SQASSERT(controller != nullptr, "");
        const auto input = controller->get_input();

        handle_input_movement(input);
        handle_input_actions(input);
    }

    //--------------------------------------------------------//

    if (fighter.current.state != State::EditorPreview)
        update_after_input();

    //--------------------------------------------------------//

    const Vec3F position ( current.position, 0.f );
    const float rotY = fighter.current.state == State::EditorPreview ? 0.5f : 0.25f * float(fighter.current.facing);
    const QuatF rotation = QuatF(0.f, rotY, 0.f);

    fighter.mModelMatrix = maths::transform(position, rotation, Vec3F(1.f));
}

//============================================================================//

void PrivateFighter::base_tick_animation()
{
    SQASSERT(fighter.current.state == State::EditorPreview || bool(mAnimation) != bool(mStaticPose),
             "fighter should always have an animation xor a static pose");

    if (mAnimation != nullptr)
    {
        SWITCH (mAnimation->mode) {

        //--------------------------------------------------------//

        // standard animations, these can loop, and support mAnimEndRootMotion and mAnimEndAboutFace
        CASE (Standard)
        {
            current.pose = armature.compute_pose(mAnimation->anim, float(mAnimTimeDiscrete));

            if (++mAnimTimeDiscrete >= int(mAnimation->anim.totalTime))
            {
                if (mAnimation->anim.times.back() == 0u)
                {
                    if (mNextAnimation == nullptr)
                        mStaticPose = &mAnimation->anim.poses.back();

                    mAnimation = mNextAnimation;
                    mNextAnimation = nullptr;

                    mAnimTimeDiscrete = 0;
                    mAnimTimeContinuous = 0.f;
                }
                else mAnimTimeDiscrete = 0;
            }
        }

        // update these animations based on horizontal velocity
        CASE (WalkCycle, DashCycle)
        {
            const auto update_movement_cycle = [&](float stride)
            {
                const float distance = std::abs(fighter.mVelocity.x);
                const float animSpeed = stride / float(mAnimation->anim.totalTime);

                mAnimTimeContinuous += distance / animSpeed / STS_TICK_RATE;
                current.pose = armature.compute_pose(mAnimation->anim, mAnimTimeContinuous);
            };

            if (mAnimation->mode == AnimMode::WalkCycle)
                update_movement_cycle(fighter.stats.anim_walk_stride);

            if (mAnimation->mode == AnimMode::DashCycle)
                update_movement_cycle(fighter.stats.anim_dash_stride);
        }

        // update these animations based vertical velocity difference from jump velocity
        CASE (JumpAscend)
        {
            const float range = mJumpVelocity + fighter.stats.fall_speed * STS_FALL_SPEED * 0.75f;
            const float progress = (mJumpVelocity - fighter.mVelocity.y) / range;

            mAnimTimeContinuous = progress * float(mAnimation->anim.totalTime);

            //sq::log_info("init: %.6f | range: %.6f | velocity: %.6f | progress: %.6f | time: %.6f | end: %.6f", mJumpVelocity, range, fighter.mVelocity.y, progress, mAnimTimeContinuous, progress + fighter.stats.gravity * STS_GRAVITY / range);

            current.pose = armature.compute_pose(mAnimation->anim, mAnimTimeContinuous);

            // if we reach fall speed next frame, then we end the animation
            if (progress + fighter.stats.gravity * STS_GRAVITY / range >= 1.f)
            {
                SQASSERT(mAnimation->anim.times.back() == 0u, "JumpAscend anims cannot loop");
                SQASSERT(mNextAnimation != nullptr, "JumpAscend anims cannot end with a static pose");

                mAnimation = mNextAnimation;
                mNextAnimation = nullptr;

                mAnimTimeDiscrete = 0;
                mAnimTimeContinuous = 0.f;
            }
        }

        // update these animations based horizontal velocity difference from brake velocity
        CASE (BrakeSlow)
        {
            const float progress = ((mBrakeVelocity - std::abs(fighter.mVelocity.x)) / mBrakeVelocity);

            mAnimTimeContinuous = progress * float(mAnimation->anim.totalTime);

            //sq::log_info("%d | init: %.6f | velocity: %.6f | progress: %.6f | time: %.6f", mAnimation, mBrakeVelocity, fighter.mVelocity.x, progress, mAnimTimeContinuous);

            current.pose = armature.compute_pose(mAnimation->anim, mAnimTimeContinuous);

            // we don't need to end this animation, as we will always switch out of it when velocity reaches zero
        }

        // continuously apply root motion to the fighter
        CASE (ApplyMotion)
        {
            if (mAnimTimeDiscrete == 0u) mPrevRootMotionOffset = Vec2F();

            current.pose = armature.compute_pose(mAnimation->anim, float(mAnimTimeDiscrete));

            const Vec3F rootOffsetLocal = current.pose.front().offset;
            const Vec3F rootOffsetWorld = { -rootOffsetLocal.z, rootOffsetLocal.x, rootOffsetLocal.y };

            fighter.mTranslate = Vec2F(rootOffsetWorld) - mPrevRootMotionOffset;
            fighter.mTranslate.x *= -float(fighter.current.facing);

            mPrevRootMotionOffset = Vec2F(rootOffsetWorld);

            //sq::log_info("%s", rootOffsetLocal);

            current.pose[0].offset = Vec3F();

            if (++mAnimTimeDiscrete >= int(mAnimation->anim.totalTime))
            {
                if (mAnimation->anim.times.back() == 0u)
                {
                    if (mNextAnimation == nullptr)
                        mStaticPose = &mAnimation->anim.poses.back();

                    mAnimation = mNextAnimation;
                    mNextAnimation = nullptr;

                    mAnimTimeDiscrete = 0;
                    mAnimTimeContinuous = 0.f;
                }
                else mAnimTimeDiscrete = 0;
            }
        }

        // Apply root motion at the end of the animation
        CASE (FixedMotion)
        {
            current.pose = armature.compute_pose(mAnimation->anim, float(mAnimTimeDiscrete));

            if (++mAnimTimeDiscrete >= int(mAnimation->anim.totalTime))
            {
                SQASSERT(mAnimation->anim.times.back() == 0u, "FixedMotion does not work for loop anims");

                const Mat4F rootTransform = armature.compute_transform(mAnimation->anim.poses.back(), 0u);

                fighter.mTranslate = Vec2F(rootTransform[3]);
                fighter.mTranslate.x *= -float(fighter.current.facing);

                //sq::log_info("%s", rootTransform[3]);

                if (mNextAnimation == nullptr)
                    mStaticPose = &mAnimation->anim.poses.back();

                mAnimation = mNextAnimation;
                mNextAnimation = nullptr;

                mAnimTimeDiscrete = 0;
                mAnimTimeContinuous = 0.f;
            }
        }

        // currently only used in the editor
        CASE (Manual) current.pose = mAnimation->anim.poses.back();

        //--------------------------------------------------------//

        } SWITCH_END;
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

    armature.compute_ubo_data(current.pose, fighter.mBoneMatrices.data(), fighter.mBoneMatrices.size());
}
