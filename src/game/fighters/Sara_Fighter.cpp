#include "game/fighters/Sara_Actions.hpp"
#include "game/fighters/Sara_Fighter.hpp"

using namespace sts;

//============================================================================//

Sara_Fighter::Sara_Fighter(Controller& controller) : Fighter("Sara", controller)
{
    actions = std::make_unique<Sara_Actions>(*this);

    //--------------------------------------------------------//

    armature.load_bones("fighters/Sara/armature.txt");
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

    if (actions->active.type == Actions::Type::Neutral_First)
    {
        animationProgress = 0.f;
        currentPose = POSE_Act_Neutral;
    }

    if (actions->active.type == Actions::Type::Tilt_Down)
    {
        animationProgress = 0.f;
        currentPose = POSE_Act_TiltDown;
    }

//    if (actions->active.type == Actions::Type::Tilt_Forward)
//    {
//        mAnimationProgress = 0.f;
//        mPoseCurrent = POSE_Act_TiltForward;
//    }

    if (actions->active.type == Actions::Type::Tilt_Up)
    {
        animationProgress = 0.f;
        currentPose = POSE_Act_TiltUp;
    }
}
