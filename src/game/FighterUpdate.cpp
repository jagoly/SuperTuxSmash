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
    // attach anim handles movement on its own
    if (mAnimation != nullptr && mAnimation->attach == true)
        return;

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
                          !(input.intX <= -3 && translation.x < -0.f) && !(input.intX >= +3 && translation.x > +0.f);

    // todo: move this to a variable for states to set from wren
    const bool ignorePlatforms = input.intY <= -3 &&
                                 (stateName == "Fall" || stateName == "FallStun" || stateName == "Helpless");

    const MoveAttempt moveAttempt = stage.attempt_move(localDiamond, vars.position, targetPosition, edgeStop, ignorePlatforms);

    vars.position = moveAttempt.result;
    vars.onPlatform = moveAttempt.onPlatform;
    vars.edge = moveAttempt.edge;

    // todo: teching, bouncing (when launched)

    if (moveAttempt.collideFloor == true)
    {
        // prevent getting caught moving up over corners
        if ((vars.onGround |= vars.velocity.y <= 0.f) == true)
            vars.velocity.y = 0.f;
    }
    else vars.onGround = false;

    if (moveAttempt.collideCeiling == true)
        vars.velocity.y = 0.f;

    // todo: I have a feeling smash doesn't let you accelerate into a wall
    //if (moveAttempt.collideWall == true)
    //    vars.velocity.x = 0.f;
}

//============================================================================//

void Fighter::update_misc()
{
    Variables& vars = variables;
    const TinyString& stateName = activeState->name;

    //-- decay or regenerate our shield ----------------------//

    // todo: this stuff should be wren, but I still need a good way to do constants

    if (stateName == "Shield")
        vars.shield = maths::max(vars.shield - SHIELD_DECAY, 0.f);

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

    debugPreviousPoseInfo = std::move(debugCurrentPoseInfo);

    //--------------------------------------------------------//

    const auto fade = mFadeProgress != mFadeFrames ?
        std::optional(float(++mFadeProgress) / float(mFadeFrames + 1u)) : std::nullopt;

    if (mAnimation->walk == true || mAnimation->dash == true)
    {
        current.pose = mArmature.compute_pose_continuous(mAnimation->anim, mAnimTimeContinuous);
        debugCurrentPoseInfo = "{} ({:.1f})"_format(mAnimation->get_key(), mAnimTimeContinuous);

        // locomotion anims usually contain root motion, but we don't need it
        current.pose[0].offset = Vec3F();

        if (variables.freezeTime == 0u)
            mAnimTimeContinuous +=
                std::abs(variables.velocity.x) * float(mAnimation->anim.frameCount) /
                (mAnimation->walk ? attributes.walkAnimStride : attributes.dashAnimStride);
    }
    else
    {
        current.pose = mArmature.compute_pose_discrete(mAnimation->anim, mAnimTimeDiscrete);
        debugCurrentPoseInfo = "{} ({} / {})"_format(mAnimation->get_key(), mAnimTimeDiscrete, mAnimation->anim.frameCount);

        if (mAnimation->motion == true)
        {
            const Vec3F offsetDiff = current.pose[0].offset - mRootMotionPreviousOffset;
            mRootMotionPreviousOffset = current.pose[0].offset;
            current.pose[0].offset = Vec3F();

            // used next frame by update_movement to actually update position
            mRootMotionTranslate = Vec2F(offsetDiff.z * float(variables.facing), offsetDiff.y);
            if (mAnimation->turn) mRootMotionTranslate.x *= -1.f;

            // apply to visual transform immediately
            current.translation += Vec3F(mRootMotionTranslate, 0.f);
        }

        if (mAnimation->turn == true)
        {
            constexpr QuatF restRotate = { 0.f, 0.70710677f, 0.70710677f, 0.f };
            constexpr QuatF invRestRotate = { 0.70710677f, 0.f, 0.f, 0.70710677f };

            // rotation fade is the same as for non-turn anims
            current.rotation = restRotate * current.pose[2].rotation * invRestRotate * current.rotation;
            current.pose[2].rotation = QuatF();
        }

        if (mAnimation->attach == true)
        {
            // we can just set position right away, unlike motion anims
            variables.position = variables.attachPoint +
                Vec2F(current.pose[0].offset.z * float(variables.facing), current.pose[0].offset.y);

            current.pose[0].offset = Vec3F();

            // fading position for attach anims is good, unlike motion anims
            if (fade.has_value() == true)
                variables.position = maths::mix(mFadeStartPosition, variables.position, *fade);

            current.translation = Vec3F(variables.position, 0.f);
        }

        if (variables.freezeTime == 0u && ++mAnimTimeDiscrete == mAnimation->anim.frameCount)
        {
            if (mAnimation->loop == false)
            {
                if (mNextAnimation == nullptr) mAnimation = nullptr;
                else play_animation(*mNextAnimation, mNextFadeFrames, true);
            }
            else mAnimTimeDiscrete = 0u;
        }
    }

    //--------------------------------------------------------//

    if (fade.has_value() == true)
    {
        if (bool(mRotateMode & RotateMode::Animation))
        {
            const float angleDiff = bool(mRotateMode & RotateMode::Clockwise) ? +0.5f : -0.5f;
            const QuatF guide = mFadeStartRotation * QuatF(0.f, angleDiff, 0.f);
            current.rotation = maths::lerp_guided(mFadeStartRotation, current.rotation, *fade, guide);
        }
        else if (mRotateMode == RotateMode::Auto)
            current.rotation = maths::lerp_shortest(mFadeStartRotation, current.rotation, *fade);

        current.pose = mArmature.blend_poses(mFadeStartPose, current.pose, *fade);
        debugAnimationFadeInfo = "{} / {}"_format(mFadeProgress, mFadeFrames + 1u);
    }
    else
    {
        mFadeProgress = mFadeFrames = 0u;
        debugAnimationFadeInfo = "No Fade";

        if (bool(mRotateMode & RotateMode::Playing))
            mRotateMode = mRotateMode | RotateMode::Done;
    }
}

//============================================================================//

void Fighter::update_frozen()
{
    debugPreviousPoseInfo = debugCurrentPoseInfo;

    if (variables.flinch == true && activeState->name != "ShieldStun")
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
        update_misc();
        update_action();
        update_state();
        update_animation();
    }
    else update_frozen();

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
