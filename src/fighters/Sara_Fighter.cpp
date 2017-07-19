#include <sqee/maths/Functions.hpp>

#include "game/FightSystem.hpp"

#include "fighters/Sara_Actions.hpp"
#include "fighters/Sara_Fighter.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Sara_Fighter::Sara_Fighter(uint8_t index, FightSystem& system, Controller& controller)
    : Fighter(index, system, controller, "Sara")
{
    mActions = create_actions(mFightSystem, *this);

    //--------------------------------------------------------//

    mBlobInfos.push_back({ 17u, { 0.f, 1.66f, 0.f }, 0.15f });
    mBlobInfos.push_back({ 17u, { 0.f, 1.82f, -0.01f }, 0.16f });

    //--------------------------------------------------------//

    for ([[maybe_unused]] const auto& blobInfo : mBlobInfos)
        mHurtBlobs.push_back(system.create_damageable_hit_blob(*this));

    //--------------------------------------------------------//

    armature.load_bones("fighters/Sara/armature.txt", true);
    armature.load_rest_pose("fighters/Sara/poses/Rest.txt");

    //--------------------------------------------------------//

    POSE_Rest = armature.make_pose("fighters/Sara/poses/Rest.txt");
    POSE_Stand = armature.make_pose("fighters/Sara/poses/Stand.txt");
    POSE_Jump = armature.make_pose("fighters/Sara/poses/Jump.txt");

    POSE_Act_Neutral = armature.make_pose("fighters/Sara/poses/Act_Neutral.txt");
    POSE_Act_TiltDown = armature.make_pose("fighters/Sara/poses/Act_TiltDown.txt");
    POSE_Act_TiltForward = armature.make_pose("fighters/Sara/poses/Act_TiltForward.txt");
    POSE_Act_TiltUp = armature.make_pose("fighters/Sara/poses/Act_TiltUp.txt");

    ANIM_Walk = armature.make_animation("fighters/Sara/anims/Walk.txt");

    //--------------------------------------------------------//

    previousPose = currentPose = POSE_Rest;
}

//============================================================================//

void Sara_Fighter::tick()
{
    this->base_tick_entity();
    this->base_tick_fighter();

    //--------------------------------------------------------//

    previousPose = currentPose;

    //--------------------------------------------------------//

    if (state.move == State::Move::None)
    {
        animationProgress = 0.f;
        currentPose = POSE_Stand;
    }

    if (state.move == State::Move::Walking)
    {
        // todo: work out exact walk animation speed
        animationProgress += std::abs(mVelocity.x) / 2.f;
        currentPose = armature.compute_pose(ANIM_Walk, animationProgress);
    }

    if (state.move == State::Move::Dashing)
    {
        // todo: work out exact dash animation speed
        animationProgress += std::abs(mVelocity.x) / 2.f;
        currentPose = armature.compute_pose(ANIM_Walk, animationProgress);
    }

    if (state.move == State::Move::Jumping)
    {
        animationProgress = 0.f;
        currentPose = POSE_Jump;
    }

    if (mActions->active_type() == Action::Type::Neutral_First)
    {
        animationProgress = 0.f;
        currentPose = POSE_Act_Neutral;
    }

    if (mActions->active_type() == Action::Type::Tilt_Down)
    {
        animationProgress = 0.f;
        currentPose = POSE_Act_TiltDown;
    }

//    if (actions->active_type() == Action::Type::Tilt_Forward)
//    {
//        mAnimationProgress = 0.f;
//        mPoseCurrent = POSE_Act_TiltForward;
//    }

    if (mActions->active_type() == Action::Type::Tilt_Up)
    {
        animationProgress = 0.f;
        currentPose = POSE_Act_TiltUp;
    }

    //--------------------------------------------------------//

    // this is really quick and dirty code, so gross :(

    const Vec3F position = Vec3F(mCurrentPosition, 0.f);
    const QuatF rotation = QuatF(0.f, 0.25f * float(state.direction), 0.f);

    const Mat4F modelMatrix = maths::transform(position, rotation, Vec3F(1.f));

    const auto matrices = armature.compute_ubo_data(currentPose);

    for (uint i = 0u; i < mBlobInfos.size(); ++i)
    {
        const auto& blobInfo = mBlobInfos[i];
        const Mat4F boneMatrix = maths::transpose(Mat4F(matrices[blobInfo.index]));
        Vec3F newOrigin = Vec3F(modelMatrix * boneMatrix * Vec4F(blobInfo.origin, 1.f));

        mHurtBlobs[i]->sphere.origin = newOrigin;
        mHurtBlobs[i]->sphere.radius = blobInfo.radius;
    }
}
