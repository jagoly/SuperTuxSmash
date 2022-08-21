#include "game/Entity.hpp"

#include "game/EntityDef.hpp"
#include "game/HitBlob.hpp"
#include "game/World.hpp"

#include "render/Renderer.hpp"

#include <sqee/maths/Functions.hpp>

using namespace sts;

//============================================================================//

Entity::Entity(const EntityDef& def)
    : world(def.world)
    , eid(def.world.generate_entity_id())
    , mAnimPlayer(def.armature)
{}

Entity::~Entity() = default;

//============================================================================//

Mat4F Entity::get_bone_matrix(int8_t index) const
{
    SQASSERT(index >= -1 && index < int8_t(mAnimPlayer.armature.get_bone_count()), "invalid index");

    if (index == -1) return mModelMatrix;

    // todo: in smash, it seems that scale and translation are ignored for the leaf bone
    // rukai data seems to ignore them for ALL bones in the chain, but that seems really wrong to me
    // see https://github.com/rukai/brawllib_rs/blob/main/src/high_level_fighter.rs#L537

    return mAnimPlayer.armature.compute_model_matrix(mAnimPlayer.currentSample, mModelMatrix, uint8_t(index));
}

Mat4F Entity::get_blended_bone_matrix(int8_t index) const
{
    SQASSERT(index >= -1 && index < int8_t(mAnimPlayer.armature.get_bone_count()), "invalid index");

    const auto matrices = reinterpret_cast<const Mat34F*>(world.renderer.ubos.matrices.map_only());

    return maths::transpose(Mat4F(matrices[mAnimPlayer.modelMatsIndex + index + 1]));
}

//============================================================================//

void Entity::play_animation(const Animation& animation, uint fade, bool fromStart)
{
    EntityVars& vars = get_vars();

    mAnimPlayer.animation = &animation;
    mNextAnimation = nullptr;

    if (bool(mRotateMode & RotateMode::Animation))
    {
        // end rotation if play_animation gets called twice
        if (bool(mRotateMode & RotateMode::Playing)) mRotateMode = RotateMode::Auto;
        else mRotateMode = mRotateMode | RotateMode::Playing;
    }

    mFadeFrames = fade;
    mNextFadeFrames = 0u;
    mFadeProgress = 0u;

    if (fade != 0u)
    {
        if (animation.attach == true)
            mFadeStartPosition = vars.position;

        if (mRotateMode == RotateMode::Auto)
            mFadeStartRotation = current.rotation;

        mFadeStartSample = mAnimPlayer.currentSample;
    }

    if (fromStart == true)
    {
        mAnimPlayer.animTime = 0.f;
        mRootMotionPreviousOffset = Vec3F();
    }

    vars.animTime = mAnimPlayer.animTime;
}

void Entity::set_next_animation(const Animation& animation, uint fade)
{
    if (mAnimPlayer.animation != nullptr)
    {
        mNextAnimation = &animation;
        mNextFadeFrames = fade;
    }
    else play_animation(animation, fade, true);
}

//============================================================================//

void Entity::update_animation()
{
    const EntityDef& def = get_def();
    EntityVars& vars = get_vars();

    current.translation = Vec3F(vars.position, 0.f);

    // only start applying slow rotation after freeze time ends
    if (vars.freezeTime == 0u)
    {
        if (bool(mRotateMode & RotateMode::Slow) && mRotateSlowTime != 0u)
        {
            const float blend = float(++mRotateSlowProgress) / float(mRotateSlowTime + 1u);

            const float angleStart = -0.25f * float(vars.facing);
            const float angleDiff = (bool(mRotateMode & RotateMode::Clockwise) ? +0.5f : -0.5f) * blend;

            current.rotation = QuatF(0.f, angleStart + angleDiff, 0.f);

            if (mRotateSlowProgress == mRotateSlowTime)
                mRotateMode = mRotateMode | RotateMode::Done;
        }
        else current.rotation = QuatF(0.f, 0.25f * float(vars.facing), 0.f);
    }

    //--------------------------------------------------------//

    // this will only happen if an action and animation don't match up
    // mainly this means fighters who don't have all of their animations yet
    if (mAnimPlayer.animation == nullptr)
    {
        if (mNextAnimation == nullptr)
        {
            mAnimPlayer.currentSample = def.armature.get_rest_sample();
            debugCurrentPoseInfo = "null";
            return;
        }

        mAnimPlayer.animation = mNextAnimation;
        mNextAnimation = nullptr;
    }

    debugPreviousPoseInfo = std::move(debugCurrentPoseInfo);

    //--------------------------------------------------------//

    const auto fade = mFadeProgress != mFadeFrames ?
        std::optional(float(++mFadeProgress) / float(mFadeFrames + 1u)) : std::nullopt;

    sq::Armature::Bone* currentSampleBones = reinterpret_cast<sq::Armature::Bone*>(mAnimPlayer.currentSample.data());

    // todo: manual flag should be made to work along side other flags
    if (mAnimPlayer.animation->manual == true)
    {
        mAnimPlayer.animTime = vars.animTime;

        def.armature.compute_sample(mAnimPlayer.animation->anim, mAnimPlayer.animTime, mAnimPlayer.currentSample);
        debugCurrentPoseInfo = fmt::format("{} ({:.3f})", mAnimPlayer.animation->get_key(), mAnimPlayer.animTime);

        // todo: locomotion hack, need an option in the export script to just not export root transforms
        currentSampleBones[0].offset = Vec3F();
    }
    else
    {
        def.armature.compute_sample(mAnimPlayer.animation->anim, mAnimPlayer.animTime, mAnimPlayer.currentSample);
        debugCurrentPoseInfo = fmt::format (
            "{} ({} / {})", mAnimPlayer.animation->get_key(), mAnimPlayer.animTime, mAnimPlayer.animation->anim.frameCount
        );

        if (mAnimPlayer.animation->motion == true)
        {
            const Vec3F offsetDiff = currentSampleBones[0].offset - mRootMotionPreviousOffset;
            mRootMotionPreviousOffset = currentSampleBones[0].offset;
            currentSampleBones[0].offset = Vec3F();

            // used next frame by update_movement to actually update position
            mRootMotionTranslate = Vec2F(offsetDiff.z * float(vars.facing), offsetDiff.y);
            if (mAnimPlayer.animation->turn) mRootMotionTranslate.x *= -1.f;

            // apply to visual transform immediately
            current.translation += Vec3F(mRootMotionTranslate, 0.f);
        }

        if (mAnimPlayer.animation->turn == true)
        {
            constexpr QuatF restRotate = { 0.f, 0.70710677f, 0.70710677f, 0.f };
            constexpr QuatF invRestRotate = { 0.70710677f, 0.f, 0.f, 0.70710677f };

            // rotation fade is the same as for non-turn anims
            current.rotation = restRotate * currentSampleBones[2].rotation * invRestRotate * current.rotation;
            currentSampleBones[2].rotation = QuatF();
        }

        if (mAnimPlayer.animation->attach == true)
        {
            // we can just set position right away, unlike motion anims
            vars.position = vars.attachPoint +
                Vec2F(currentSampleBones[0].offset.z * float(vars.facing), currentSampleBones[0].offset.y);

            currentSampleBones[0].offset = Vec3F();

            // fading position for attach anims is good, unlike motion anims
            if (fade.has_value() == true)
                vars.position = maths::mix(mFadeStartPosition, vars.position, *fade);

            current.translation = Vec3F(vars.position, 0.f);
        }

        if (vars.freezeTime == 0u && ++mAnimPlayer.animTime == mAnimPlayer.animation->anim.frameCount)
        {
            if (mAnimPlayer.animation->loop == false)
            {
                if (mNextAnimation == nullptr) mAnimPlayer.animation = nullptr;
                else play_animation(*mNextAnimation, mNextFadeFrames, true);
            }
            else mAnimPlayer.animTime = 0.f;
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

        def.armature.blend_samples(mFadeStartSample, mAnimPlayer.currentSample, *fade, mAnimPlayer.currentSample);
        debugAnimationFadeInfo = fmt::format("{} / {}", mFadeProgress, mFadeFrames + 1u);
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

void Entity::integrate_base(float blend)
{
    EntityVars& vars = get_vars();

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

    const Mat4F modelMatrix = maths::transform(translation, rotation);

    mAnimPlayer.integrate(world.renderer, modelMatrix, vars.facing, blend);
}
