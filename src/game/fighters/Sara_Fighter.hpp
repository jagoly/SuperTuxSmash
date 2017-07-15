#pragma once

#include <sqee/render/Armature.hpp>

#include "game/Fighter.hpp"

//============================================================================//

namespace sts {

class Sara_Fighter final : public Fighter
{
public: //====================================================//

    Sara_Fighter(Controller& controller);

    //--------------------------------------------------------//

    void tick() override;

private: //===================================================//

    sq::Armature armature;

    sq::Armature::Pose previousPose;
    sq::Armature::Pose currentPose;

    float animationProgress = 0.f;

    //--------------------------------------------------------//

    sq::Armature::Pose POSE_Rest;
    sq::Armature::Pose POSE_Stand;
    sq::Armature::Pose POSE_Jump;

    sq::Armature::Pose POSE_Act_Neutral;
    sq::Armature::Pose POSE_Act_TiltDown;
    sq::Armature::Pose POSE_Act_TiltForward;
    sq::Armature::Pose POSE_Act_TiltUp;

    sq::Armature::Animation ANIM_Walk;

    //--------------------------------------------------------//

    friend class Sara_Render;
};

} // namespace sts