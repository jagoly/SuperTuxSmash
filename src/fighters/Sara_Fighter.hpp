#pragma once

#include "game/Fighter.hpp"

//============================================================================//

namespace sts {

class Sara_Fighter final : public Fighter
{
public: //====================================================//

    Sara_Fighter(uint8_t index, FightWorld& world, Controller& controller);

    //--------------------------------------------------------//

    void tick() override;

    //--------------------------------------------------------//

    sq::Armature::Pose POSE_Stand;

    //sq::Armature::Pose POSE_Jump_Ascend;
    //sq::Armature::Pose POSE_Jump_Descend;

    sq::Armature::Animation ANIM_Walk;

    sq::Armature::Animation ANIM_Jump_Start;

    sq::Armature::Animation ANIM_Action_Neutral_First;
    sq::Armature::Animation ANIM_Action_Tilt_Down;
    sq::Armature::Animation ANIM_Action_Tilt_Forward;
    sq::Armature::Animation ANIM_Action_Tilt_Up;

private: //===================================================//

    float mAnimTimeContinuous = 0.f;

    bool mJumpStartDone = false;

    //--------------------------------------------------------//

    friend class Sara_Render;
};

} // namespace sts
