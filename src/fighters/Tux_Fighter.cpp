#include "fighters/Tux_Actions.hpp"
#include "fighters/Tux_Fighter.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Tux_Fighter::Tux_Fighter(uint8_t index, FightWorld& world, Controller& controller)
    : Fighter(index, world, controller, "assets/fighters/Tux/")
{
    mActions = create_actions(mFightWorld, *this);

    //--------------------------------------------------------//

    POSE_Stand = mArmature.make_pose("assets/fighters/Tux/poses/Stand.txt");
    POSE_Slide = mArmature.make_pose("assets/fighters/Tux/poses/Slide.txt");

    //--------------------------------------------------------//

    const auto load_animation = [this](auto& anim, const string& path)
    { anim = mArmature.make_animation("assets/fighters/Tux/anims/" + path); };

    load_animation(ANIM_Walk, "Walk.txt");
}

//============================================================================//

void Tux_Fighter::tick()
{
    this->base_tick_fighter();

    //--------------------------------------------------------//

    if (mActions->active_type() == Action::Type::None)
    {
        if (state.move == State::Move::None)
        {
            mAnimTimeContinuous = 0.f;
            update_pose(POSE_Stand);
        }

        if (state.move == State::Move::Walk)
        {
            // todo: work out exact walk animation speed
            mAnimTimeContinuous += std::abs(mVelocity.x) * 0.9f;
            update_pose(ANIM_Walk, mAnimTimeContinuous);
        }

        if (state.move == State::Move::Dash)
        {
            mAnimTimeContinuous = 0.f;
            update_pose(POSE_Slide);
            // todo: work out exact dash animation speed
            //mAnimTimeContinuous += std::abs(mVelocity.x) * 0.9f;
            //update_pose(ANIM_Walk, mAnimTimeContinuous);
        }
    }

    //--------------------------------------------------------//

    if (get_animation() == nullptr)
    {
        // todo: this needs refactoring
        if (state.move == State::Move::Air)
        {
            if (mJumpStartDone == false)
            {
                //play_animation(ANIM_Jump_Start);
                mJumpStartDone = true;
            }
        }
        else mJumpStartDone = false;
    }

    //--------------------------------------------------------//

    this->base_tick_animation();
}
