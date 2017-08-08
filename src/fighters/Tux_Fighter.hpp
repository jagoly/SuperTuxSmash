#pragma once

#include "game/Fighter.hpp"

//============================================================================//

namespace sts {

class Tux_Fighter final : public Fighter
{
public: //====================================================//

    Tux_Fighter(uint8_t index, FightWorld& world, Controller& controller);

    //--------------------------------------------------------//

    void tick() override;

    //--------------------------------------------------------//

    sq::Armature::Pose POSE_Stand;
    sq::Armature::Pose POSE_Slide;

    sq::Armature::Animation ANIM_Walk;

private: //===================================================//

    float mAnimTimeContinuous = 0.f;

    bool mJumpStartDone = false;

    //--------------------------------------------------------//

    friend class Tux_Render;
};

} // namespace sts
