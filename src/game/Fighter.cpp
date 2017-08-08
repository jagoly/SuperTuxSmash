#include <sqee/assert.hpp>
#include <sqee/debug/Logging.hpp>

#include <sqee/misc/Json.hpp>
#include <sqee/maths/Functions.hpp>

#include "game/Fighter.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Fighter::Fighter(uint8_t index, FightWorld& world, Controller& controller, string path)
    : index(index), mFightWorld(world), mController(controller)
{
    state.move = State::Move::None;
    state.direction = State::Direction::Left;

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
}

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

void Fighter::impl_initialise_stats(const string& path)
{
    const auto json = sq::parse_json(path + "Stats.json");

    stats.walk_speed    = json.at("walk_speed");
    stats.dash_speed    = json.at("dash_speed");
    stats.air_speed     = json.at("air_speed");
    stats.land_traction = json.at("land_traction");
    stats.air_traction  = json.at("air_traction");
    stats.hop_height    = json.at("hop_height");
    stats.jump_height   = json.at("jump_height");
    stats.fall_speed    = json.at("fall_speed");
}

//============================================================================//

void Fighter::impl_input_movement(Controller::Input input)
{
    //-- maximum horizontal velocities (signed) --------------//

    const float maxNewWalkSpeed = input.axis_move.x * (stats.walk_speed * 5.f + stats.land_traction * 0.5f);
    const float maxNewAirSpeed = input.axis_move.x * (stats.air_speed * 5.f + stats.air_traction * 0.2f);
    const float maxDashSpeed = stats.dash_speed * 8.f + stats.land_traction * 0.5f;

    switch (state.move) {

    //== None ====================================================//

    case State::Move::None:
    {
        if (mActions->active_type() != Action::Type::None) break;

        //--------------------------------------------------------//

        if (input.axis_move.x < -0.f)
        {
            state.move = State::Move::Walk;
            state.direction = State::Direction::Left;
            mVelocity.x = -(stats.land_traction * 1.f);
        }

        if (input.axis_move.x > +0.f)
        {
            state.move = State::Move::Walk;
            state.direction = State::Direction::Right;
            mVelocity.x = +(stats.land_traction * 1.f);
        }

        //--------------------------------------------------------//

        if (input.press_jump == true)
        {
            state.move = State::Move::Air;

            mJumpHeld = true;

            const float base = stats.fall_speed * stats.jump_height;
            mVelocity.y = std::sqrt(base * 3.f * 48.f);
        }

        else if (input.activate_dash == true)
        {
            state.move = State::Move::Dash;
        }

        //--------------------------------------------------------//

        break;
    }

    //== Walking =================================================//

    case State::Move::Walk:
    {
        SQASSERT(mActions->active_type() == Action::Type::None, "");

        //--------------------------------------------------------//

        if (input.axis_move.x == 0.f)
        {
            state.move = State::Move::None;
        }

        else if (input.axis_move.x < -0.f)
        {
            state.direction = State::Direction::Left;

            if (mVelocity.x > maxNewWalkSpeed)
            {
                mVelocity.x -= stats.land_traction * 1.f;
                mVelocity.x = maths::max(mVelocity.x, maxNewWalkSpeed);
            }
        }

        else if (input.axis_move.x > +0.f)
        {
            state.direction = State::Direction::Right;

            if (mVelocity.x < maxNewWalkSpeed)
            {
                mVelocity.x += stats.land_traction * 1.f;
                mVelocity.x = maths::min(mVelocity.x, maxNewWalkSpeed);
            }
        }

        //--------------------------------------------------------//

        if (input.press_jump == true)
        {
            state.move = State::Move::Air;

            mJumpHeld = true;

            const float base = stats.fall_speed * stats.jump_height;
            mVelocity.y = std::sqrt(base * 3.f * 48.f);
        }

        else if (input.activate_dash == true)
        {
            state.move = State::Move::Dash;
        }

        //--------------------------------------------------------//

        break;
    }

    //== Dashing =================================================//

    case State::Move::Dash:
    {
        SQASSERT(mActions->active_type() == Action::Type::None, "");

        //--------------------------------------------------------//

        if (input.axis_move.x == 0.f)
        {
            state.move = State::Move::None;
        }

        else if (input.axis_move.x < -0.f)
        {
            if (mVelocity.x > -maxDashSpeed)
            {
                mVelocity.x -= stats.land_traction * 1.5f;
                mVelocity.x = maths::max(mVelocity.x, -maxDashSpeed);
            }

            if (input.axis_move.x > -0.8f) state.move = State::Move::Walk;
        }

        else if (input.axis_move.x > +0.f)
        {
            if (mVelocity.x < +maxDashSpeed)
            {
                mVelocity.x += stats.land_traction * 1.5f;
                mVelocity.x = maths::min(mVelocity.x, +maxDashSpeed);
            }

            if (input.axis_move.x < +0.8f) state.move = State::Move::Walk;
        }

        //--------------------------------------------------------//

        if (input.press_jump == true)
        {
            state.move = State::Move::Air;

            mJumpHeld = true;

            const float base = stats.fall_speed * stats.jump_height;
            mVelocity.y = std::sqrt(base * 3.f * 48.f);
        }

        //--------------------------------------------------------//

        break;
    }

    //== Aerial ==================================================//

    case State::Move::Air:
    {
        mJumpHeld = mJumpHeld && input.hold_jump;

        if (input.axis_move.x < -0.f)
        {
            if (mVelocity.x > maxNewAirSpeed)
            {
                mVelocity.x -= stats.air_traction * 0.4f;
                mVelocity.x = maths::max(mVelocity.x, maxNewAirSpeed);
            }
        }

        if (input.axis_move.x > +0.f)
        {
            if (mVelocity.x < maxNewAirSpeed)
            {
                mVelocity.x += stats.air_traction * 0.4f;
                mVelocity.x = maths::min(mVelocity.x, maxNewAirSpeed);
            }
        }

        if (input.axis_move.y < -0.f)
        {
            const float base = stats.jump_height * stats.fall_speed;
            mVelocity.y += base * input.axis_move.y * 0.1f;
        }

        if (input.axis_move.y > +0.f)
        {
            const float base = stats.jump_height * stats.fall_speed;
            mVelocity.y += base * input.axis_move.y * 0.1f;
        }

        break;
    }

    //== Knocked =================================================//

    case State::Move::Knock:
    {
        break;
    }

    //============================================================//

    } // switch (state.move)
}

//============================================================================//

void Fighter::impl_input_actions(Controller::Input input)
{
    if (mActions->active_type() != Action::Type::None)
        return;

    Action::Type target = Action::Type::None;

    //--------------------------------------------------------//

    if (input.press_attack == true)
    {
        const Vec2F tilt = input.axis_move;

        const Vec2F absTilt = maths::abs(tilt);
        const bool compareXY = absTilt.x > absTilt.y;

        const bool signX = std::signbit(tilt.x);
        const bool signY = std::signbit(tilt.y);

        //--------------------------------------------------------//

        if (state.move == State::Move::None)
        {
            if (tilt.y == 0.f) target = Action::Type::Neutral_First;
            else target = signY ? Action::Type::Tilt_Down : Action::Type::Tilt_Up;
        }

        //--------------------------------------------------------//

        else if (state.move == State::Move::Walk)
        {
            state.move = State::Move::None;

            if (compareXY == true) target = Action::Type::Tilt_Forward;
            else target = signY ? Action::Type::Tilt_Down : Action::Type::Tilt_Up;
        }

        //--------------------------------------------------------//

        else if (state.move == State::Move::Dash)
        {
            state.move = State::Move::None;

            target = Action::Type::Dash_Attack;
        }

        //--------------------------------------------------------//

        else if (state.move == State::Move::Air)
        {
            if (tilt == Vec2F(0.f, 0.f)) target = Action::Type::Air_Neutral;

            else if (compareXY && state.direction == State::Direction::Left)
                target = signX ? Action::Type::Air_Forward : Action::Type::Air_Back;

            else if (compareXY && state.direction == State::Direction::Right)
                target = signX ? Action::Type::Air_Back : Action::Type::Air_Forward;

            else target = signY ? Action::Type::Air_Down : Action::Type::Air_Up;
        }

        //--------------------------------------------------------//

        else if (state.move == State::Move::Knock)
        {

        }
    }

    //--------------------------------------------------------//

    if (target != Action::Type::None)
    {
        mActions->switch_active(target);
    }
}

//============================================================================//

void Fighter::impl_update_physics()
{
    //-- apply friction --------------------------------------//

    if (state.move == State::Move::None || state.move == State::Move::Walk || state.move == State::Move::Dash)
    {
        if (mVelocity.x < -0.f) mVelocity.x = maths::min(mVelocity.x + (stats.land_traction * 0.5f), -0.f);
        if (mVelocity.x > +0.f) mVelocity.x = maths::max(mVelocity.x - (stats.land_traction * 0.5f), +0.f);
    }

    if (state.move == State::Move::Air || state.move == State::Move::Knock)
    {
        if (mVelocity.x < -0.f) mVelocity.x = maths::min(mVelocity.x + (stats.air_traction * 0.2f), -0.f);
        if (mVelocity.x > +0.f) mVelocity.x = maths::max(mVelocity.x - (stats.air_traction * 0.2f), +0.f);
    }

    //-- apply gravity ---------------------------------------//

    if (state.move == State::Move::Air || state.move == State::Move::Knock)
    {
        const float fraction = (2.f/3.f) * stats.hop_height / stats.jump_height;

        float gravity = stats.fall_speed * 0.5f;
        if (!mJumpHeld && mVelocity.y > 0.f) gravity /= fraction;
        mVelocity.y -= gravity;

        const float maxFall = stats.fall_speed * 15.f;
        mVelocity.y = maths::max(mVelocity.y, -maxFall);
    }

    //-- update position -------------------------------------//

    current.position += mVelocity / 48.f;

    //-- check landing ---------------------------------------//

    if (state.move == State::Move::Air)
    {
        if (current.position.y <= 0.f)
        {
            mVelocity.y = current.position.y = 0.f;
            state.move = State::Move::None;
            // normal landing
        }
    }

    if (state.move == State::Move::Knock)
    {
        if (current.position.y <= 0.f)
        {
            mVelocity.y = current.position.y = 0.f;
            state.move = State::Move::None;
            // splat on the ground
        }
    }
}

//============================================================================//

void Fighter::base_tick_fighter()
{
    previous = current;

    //--------------------------------------------------------//

    auto input = mController.get_input();

    impl_input_movement(input);
    impl_input_actions(input);

    //--------------------------------------------------------//

    impl_update_physics();

    mActions->tick_active_action();

    //--------------------------------------------------------//

    const Vec3F position ( current.position, 0.f );
    const QuatF rotation ( 0.f, 0.25f * float(state.direction), 0.f );

    mModelMatrix = maths::transform(position, rotation, Vec3F(1.f));
}

//============================================================================//

void Fighter::base_tick_animation()
{
    if (mAnimation != nullptr)
    {
        update_pose(*mAnimation, float(mAnimationTime));

        if (++mAnimationTime == mAnimation->totalTime)
            mAnimation = nullptr;
    }

    mBoneMatrices = mArmature.compute_ubo_data(current.pose);
}

//============================================================================//

void Fighter::update_pose(const sq::Armature::Pose& pose)
{
    current.pose = pose;
}

void Fighter::update_pose(const sq::Armature::Animation& anim, float time)
{
    current.pose = mArmature.compute_pose(anim, time);
}

void Fighter::play_animation(const sq::Armature::Animation& anim)
{
    mAnimation = &anim;
    mAnimationTime = 0u;
}

//============================================================================//

void Fighter::apply_hit_basic(const HitBlob& hit)
{
    const float angle = maths::radians(hit.knockAngle * float(hit.fighter->state.direction));
    const Vec2F knockDir = { std::sin(angle), std::cos(angle) };

    mVelocity = maths::normalize(knockDir) * hit.knockBase;

    state.move = State::Move::Knock;
}

//============================================================================//

Mat4F Fighter::interpolate_model_matrix(float blend) const
{
    const Vec2F position = maths::mix(previous.position, current.position, blend);
    const QuatF rotation = QuatF(0.f, 0.25f * float(state.direction), 0.f);

    return maths::transform(Vec3F(position, 0.f), rotation, Vec3F(1.f));
}


std::vector<Mat34F> Fighter::interpolate_bone_matrices(float blend) const
{
    auto blendPose = mArmature.blend_poses(previous.pose, current.pose, blend);
    return mArmature.compute_ubo_data(blendPose);
}
