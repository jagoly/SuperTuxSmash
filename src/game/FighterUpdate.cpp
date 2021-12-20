#include "game/Fighter.hpp" // IWYU pragma: associated

#include "main/Options.hpp"

#include "game/Controller.hpp"
#include "game/FightWorld.hpp"
#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/Stage.hpp"

#include "render/UniformBlocks.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>

using namespace sts;

//============================================================================//

void Fighter::update_action()
{
    // editor wants to restart an action
    if (editorStartAction != nullptr)
    {
        cancel_action();
        start_action(*editorStartAction);
        editorStartAction = nullptr;
    }

    // update normally
    else if (activeAction != nullptr && activeAction != editorErrorCause)
        activeAction->call_do_updates();
}

void Fighter::update_state()
{
    // todo: editor can't deal with state errors yet, so ignore them for now
    if (activeState != nullptr /*&& activeState != editorErrorCause*/)
        activeState->call_do_updates();
}

//============================================================================//

void Fighter::update_movement()
{
    const Attributes& attrs = attributes;
    Variables& vars = variables;
    const InputFrame& input = controller->history.frames.front();
    Stage& stage = world.get_stage();

    //-- apply knockback decay -------------------------------//

    if (vars.launchSpeed != 0.f)
    {
        const float actualSpeed = maths::length(vars.velocity);
        const float decay = maths::min(vars.launchSpeed, KNOCKBACK_DECAY);

        vars.launchSpeed -= decay;

        if (actualSpeed <= decay) vars.velocity = Vec2F();
        else vars.velocity *= (actualSpeed - decay) / actualSpeed;
    }

    //-- apply friction --------------------------------------//

    if (vars.applyFriction == true)
    {
        if (vars.onGround == true)
        {
            if (vars.velocity.x < -0.f) vars.velocity.x = maths::min(vars.velocity.x + attrs.traction, -0.f);
            if (vars.velocity.x > +0.f) vars.velocity.x = maths::max(vars.velocity.x - attrs.traction, +0.f);
        }
        else if (vars.launchSpeed == 0.f && (vars.moveMobility == 0.f || input.intX == 0))
        {
            if (vars.velocity.x < -0.f) vars.velocity.x = maths::min(vars.velocity.x + attrs.airFriction, -0.f);
            if (vars.velocity.x > +0.f) vars.velocity.x = maths::max(vars.velocity.x - attrs.airFriction, +0.f);
        }
    }

    //-- apply horizontal movement ---------------------------//

    if (vars.moveMobility != 0.f && (vars.onGround == false || input.relIntX > 0))
    {
        const float targetValue = input.floatX * vars.moveSpeed;

        if (input.intX < 0 && vars.velocity.x > targetValue)
            vars.velocity.x = maths::max(vars.velocity.x - vars.moveMobility, targetValue);

        if (input.intX > 0 && vars.velocity.x < input.floatX * vars.moveSpeed)
            vars.velocity.x = maths::min(vars.velocity.x + vars.moveMobility, targetValue);
    }

    //-- apply gravity ---------------------------------------//

    if (vars.applyGravity == true)
    {
        const float targetValue = vars.fastFall ? -attrs.fastFallSpeed : -attrs.fallSpeed;

        // falling faster than maximum speed
        if (vars.velocity.y < targetValue)
        {
            // if we were meteor smashed, we have to wait for launch speed to decay on its own
            // otherwise, apply half gravity upwards (no idea what smash does)
            if (vars.launchSpeed == 0.f)
                vars.velocity.y = maths::min(vars.velocity.y + attrs.gravity * 0.5f, targetValue);
        }

        // apply gravity normally
        else if (vars.fastFall == false)
            vars.velocity.y = maths::max(vars.velocity.y - attrs.gravity, targetValue);

        // fast fall applies instantly
        else vars.velocity.y = targetValue;
    }

    // when gravity is disabled, reduce velocity towards zero
    else if (vars.velocity.y < -0.f)
        vars.velocity.y = maths::min(vars.velocity.y + attrs.gravity, -0.f);
    else if (vars.velocity.y > +0.f)
        vars.velocity.y = maths::max(vars.velocity.y - attrs.gravity, +0.f);

    //-- ask the stage where we can move ---------------------//

    const Vec2F translation = vars.velocity + mRootMotionTranslate;
    const Vec2F targetPosition = vars.position + translation;
    mRootMotionTranslate = Vec2F();

    const TinyString& stateName = activeState->name;

    const bool edgeStop = vars.onGround == false ? false :
                          vars.edgeStop == EdgeStopMode::Never ? false : vars.edgeStop == EdgeStopMode::Always ? true :
                          !(input.intX <= -3 && translation.x < -0.0f) && !(input.intX >= +3 && translation.x > +0.0f);

    // todo: move this to a variable for states to set from wren
    const bool ignorePlatforms = input.intY <= -3 &&
                                 (stateName == "Fall" || stateName == "FallStun" || stateName == "Helpless");

    const MoveAttempt moveAttempt = stage.attempt_move(localDiamond, vars.position, targetPosition, edgeStop, ignorePlatforms);

    vars.position = moveAttempt.result;
    vars.onPlatform = moveAttempt.onPlatform;

    // todo: teching, bouncing (when launched)

    if (moveAttempt.collideFloor == true)
    {
        // prevent getting caught when move up over corners
        if ((vars.onGround |= vars.velocity.y <= 0.f))
            vars.velocity.y = 0.f;
    }
    else vars.onGround = false;

    if (moveAttempt.collideCeiling == true)
        vars.velocity.y = 0.f;

    // todo: I have a feeling smash doesn't let you accelerate into a wall
    //if (moveAttempt.collideWall == true)
    //    vars.velocity.x = 0.f;

    //-- activate vertigo animation --------------------------//

    // todo: make this a state + action

    if (stateName == "Neutral" || stateName == "Walk")
    {
        if (vars.vertigo == false && moveAttempt.edge == vars.facing)
        {
            wren_play_animation("VertigoStart", 2u, true);
            wren_set_next_animation("VertigoLoop", 0u);
            vars.vertigo = true;
        }
    }
    else vars.vertigo = false;

    //-- decay or regenerate our shield ----------------------//

    // todo: move to wren

    if (stateName == "Shield")
    {
        vars.shield -= SHIELD_DECAY;
        if (vars.shield <= 0.f)
            apply_shield_break(); // todo
    }

    else if (stateName != "ShieldStun")
        vars.shield = maths::min(vars.shield + SHIELD_REGEN, SHIELD_MAX_HP);
}

//============================================================================//

void Fighter::update_animation()
{
    current.translation = Vec3F(variables.position, 0.f);

    // only start applying slow rotation after freeze time ends
    if (variables.freezeTime == 0u)
    {
        if (bool(mRotateMode & RotateMode::Slow) && mRotateSlowTime != 0u)
        {
            const float blend = float(++mRotateSlowProgress) / float(mRotateSlowTime + 1u);

            const float angleStart = -0.25f * float(variables.facing);
            const float angleDiff = (bool(mRotateMode & RotateMode::Clockwise) ? +0.5f : -0.5f) * blend;

            current.rotation = QuatF(0.f, angleStart + angleDiff, 0.f);

            if (mRotateSlowProgress == mRotateSlowTime)
                mRotateMode = mRotateMode | RotateMode::Done;
        }
        else current.rotation = QuatF(0.f, 0.25f * float(variables.facing), 0.f);
    }

    //--------------------------------------------------------//

    // this will only happen if an action and animation don't match up
    // mainly this means fighters who don't have all of their animations yet
    if (mAnimation == nullptr)
    {
        if (mNextAnimation == nullptr)
        {
            current.pose = mArmature.get_rest_pose();
            debugCurrentPoseInfo = "null";
            return;
        }
        mAnimation = mNextAnimation;
        mNextAnimation = nullptr;
    }

    //--------------------------------------------------------//

    // used for turn animations
    const QuatF restRotate = QuatF(0.f, 0.5f, 0.f) * QuatF(-0.25f, 0.f, 0.f);
    const QuatF invRestRotate = QuatF(+0.25f, 0.f, 0.f);

    // called when a non-looping animation reaches the end
    const auto finish_animation = [&]()
    {
        if (mNextAnimation == nullptr)
        {
            mAnimation = nullptr;
            mFadeProgress = mFadeFrames = 0u;
        }
        else play_animation(*mNextAnimation, mNextFadeFrames, true);
    };

    //--------------------------------------------------------//

    const auto set_current_pose_discrete = [this](const Animation& anim, uint time)
    {
        current.pose = mArmature.compute_pose_discrete(anim.anim, time);

        if (world.options.log_animation == true)
            sq::log_debug("pose: {} - {}", anim.get_key(), time);

        debugPreviousPoseInfo = std::move(debugCurrentPoseInfo);
        debugCurrentPoseInfo = "{} ({} / {})"_format(anim.get_key(), time, anim.anim.frameCount);
    };

    const auto set_current_pose_continuous = [this](const Animation& anim, float time)
    {
        current.pose = mArmature.compute_pose_continuous(anim.anim, time);

        if (world.options.log_animation == true)
            sq::log_debug("pose: {} - {}", anim.get_key(), time);

        debugPreviousPoseInfo = std::move(debugCurrentPoseInfo);
        debugCurrentPoseInfo = "{} ({:.1f})"_format(anim.get_key(), time);
    };

    //--------------------------------------------------------//

    SWITCH (mAnimation->mode) {

    CASE (Basic)
    {
        set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

        if (variables.freezeTime == 0u)
            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
                finish_animation();
    }

    CASE (Loop)
    {
        SQASSERT(mNextAnimation == nullptr, "");

        set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

        if (variables.freezeTime == 0u)
            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
                mAnimTimeDiscrete = 0u;
    }

    CASE (WalkLoop)
    {
        SQASSERT(mNextAnimation == nullptr, "");

        set_current_pose_continuous(*mAnimation, mAnimTimeContinuous);

        current.pose[0].offset = Vec3F();

        if (variables.freezeTime == 0u)
        {
            const float moveSpeed = std::abs(variables.velocity.x);
            const float animSpeed = attributes.walkAnimStride / float(mAnimation->anim.frameCount);
            mAnimTimeContinuous += moveSpeed / animSpeed;
        }
    }

    CASE (DashLoop)
    {
        SQASSERT(mNextAnimation == nullptr, "");

        set_current_pose_continuous(*mAnimation, mAnimTimeContinuous);

        current.pose[0].offset = Vec3F();

        if (variables.freezeTime == 0u)
        {
            const float moveSpeed = std::abs(variables.velocity.x);
            const float animSpeed = attributes.dashAnimStride / float(mAnimation->anim.frameCount);
            mAnimTimeContinuous += moveSpeed / animSpeed;
        }
    }

    CASE (Motion)
    {
        set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

        const Vec3F offsetLocal = current.pose[0].offset - mRootMotionPreviousOffset;
        mRootMotionPreviousOffset = current.pose[0].offset;
        current.pose[0].offset = Vec3F();

        mRootMotionTranslate = { offsetLocal.z * float(variables.facing), offsetLocal.y };
        current.translation += Vec3F(mRootMotionTranslate, 0.f);

        if (variables.freezeTime == 0u)
            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
                finish_animation();
    }

    CASE (Turn)
    {
        set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

        current.rotation = restRotate * current.pose[2].rotation * invRestRotate * current.rotation;
        current.pose[2].rotation = QuatF();

        if (variables.freezeTime == 0u)
            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
                finish_animation();
    }

    CASE (MotionTurn)
    {
        set_current_pose_discrete(*mAnimation, mAnimTimeDiscrete);

        const Vec3F offsetLocal = current.pose[0].offset - mRootMotionPreviousOffset;
        mRootMotionPreviousOffset = current.pose[0].offset;
        current.pose[0].offset = Vec3F();

        mRootMotionTranslate = { offsetLocal.z * float(variables.facing) * -1.f, offsetLocal.y };
        current.translation += Vec3F(mRootMotionTranslate, 0.f);

        current.rotation = restRotate * current.pose[2].rotation * invRestRotate * current.rotation;
        current.pose[2].rotation = QuatF();

        if (variables.freezeTime == 0u)
            if (++mAnimTimeDiscrete == mAnimation->anim.frameCount)
                finish_animation();
    }

    } SWITCH_END;

    //-- blend from fade pose for smooth transitions ---------//

    if (mFadeProgress != mFadeFrames)
    {
        const float blend = float(++mFadeProgress) / float(mFadeFrames + 1u);

        if (mRotateMode != RotateMode::Auto)
        {
            const float angleDiff = bool(mRotateMode & RotateMode::Clockwise) ? +0.5f : -0.5f;
            const QuatF guide = mFadeStartRotation * QuatF(0.f, angleDiff, 0.f);
            current.rotation = maths::lerp_guided(mFadeStartRotation, current.rotation, blend, guide);
        }
        else current.rotation = maths::lerp_shortest(mFadeStartRotation, current.rotation, blend);

        current.pose = mArmature.blend_poses(mFadeStartPose, current.pose, blend);

        if (world.options.log_animation == true)
            sq::log_debug("blend - {} / {} - {}", mFadeProgress, mFadeFrames, blend);

        debugAnimationFadeInfo = "{} / {}"_format(mFadeProgress, mFadeFrames + 1u);
    }
    else
    {
        mFadeProgress = mFadeFrames = 0u;
        debugAnimationFadeInfo = "No Fade";

        // once fade finishes we can disable animated rotation
        if (bool(mRotateMode & RotateMode::Playing))
            mRotateMode = mRotateMode | RotateMode::Done;
    }
}

//============================================================================//

void Fighter::update_frozen()
{
    debugPreviousPoseInfo = debugCurrentPoseInfo;

    if (variables.flinch == true)
    {
        constexpr auto jitterValues = std::array
        {
            Vec2F(-0.8f, -0.8f), Vec2F(-0.4f, +0.1f), Vec2F( 0.0f, +1.0f), Vec2F(+0.4f, +0.1f),
            Vec2F(+0.8f, -0.8f), Vec2F(-0.1f, -0.4f), Vec2F(-1.0f,  0.0f), Vec2F(-0.1f, +0.4f),
            Vec2F(+0.8f, +0.8f), Vec2F(+0.4f, -0.1f), Vec2F( 0.0f, -1.0f), Vec2F(-0.4f, -0.1f),
            Vec2F(-0.8f, +0.8f), Vec2F(+0.1f, +0.4f), Vec2F(+1.0f,  0.0f), Vec2F(+0.1f, -0.4f),
        };

        const uint8_t jitterIndex = mJitterCounter % 16u;
        const float jitterStrength = float(variables.freezeTime) * 0.125f / 32.f + 0.0625f;
        const Vec2F jitter = jitterValues[jitterIndex] * jitterStrength;

        current.translation = Vec3F(variables.position + jitter, 0.f);
        mJitterCounter = uint8_t(int8_t(mJitterCounter) - variables.facing);
    }

    --variables.freezeTime;
}

//============================================================================//

void Fighter::tick()
{
    previous = current;

    // set relative x for the newly added input frame
    controller->history.frames.front().set_relative_x(variables.facing);

    // check if we have crossed the stage boundary
    if (world.get_stage().check_point_out_of_bounds(variables.position + localDiamond.cross()))
        pass_boundary();

    // finish slow or animated rotation
    if (bool(mRotateMode & RotateMode::Done))
    {
        mRotateSlowTime = mRotateSlowProgress = 0u;
        mRotateMode = RotateMode::Auto;
    }

    // do updates
    if (variables.freezeTime == 0u)
    {
        update_movement();
        update_action();
        update_state();
        update_animation();
    }
    else update_frozen();

    //if (activeState->name == "EditorPreview") current.rotation = QuatF(0.f, 0.5f, 0.f);

    // compute updated matrices
    mModelMatrix = maths::transform(current.translation, current.rotation);
    mArmature.compute_ubo_data(current.pose, mBoneMatrices.data(), mBoneMatrices.size());
}

//============================================================================//

void Fighter::integrate(float blend)
{
    const Vec3F translation = maths::mix(previous.translation, current.translation, blend);
    const QuatF rotation = [&]()
    {
        if (mRotateMode != RotateMode::Auto)
        {
            const float angleDiff = bool(mRotateMode & RotateMode::Clockwise) ? +0.5f : -0.5f;
            const QuatF guide = previous.rotation * QuatF(0.f, angleDiff, 0.f);

            return maths::lerp_guided(previous.rotation, current.rotation, blend, guide);
        }

        return maths::lerp_shortest(previous.rotation, current.rotation, blend);
    }();

    mBlendedModelMatrix = maths::transform(translation, rotation);

    const sq::Armature::Pose pose = mArmature.blend_poses(previous.pose, current.pose, blend);

    mDescriptorSet.swap();
    auto& block = *reinterpret_cast<SkellyBlock*>(mSkellyUbo.swap_map());

    block.matrix = mBlendedModelMatrix;
    //block.normMat = Mat34F(maths::normal_matrix(camera.viewMat * mInterpModelMatrix));
    block.normMat = Mat34F(maths::normal_matrix(mBlendedModelMatrix));

    mArmature.compute_ubo_data(pose, block.bones, 80u);
}
