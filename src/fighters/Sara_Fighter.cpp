#include "fighters/Sara_Actions.hpp"
#include "fighters/Sara_Fighter.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Sara_Fighter::Sara_Fighter(uint8_t index, FightWorld& world, Controller& controller)
    : Fighter(index, world, controller, "assets/fighters/Sara/")
{
    mActions = create_actions(mFightWorld, *this);

    //--------------------------------------------------------//

    POSE_Stand = mArmature.make_pose("assets/fighters/Sara/poses/Stand.txt");

    //--------------------------------------------------------//

    const auto load_animation = [this](auto& anim, const string& path)
    { anim = mArmature.make_animation("assets/fighters/Sara/anims/" + path); };

    load_animation(ANIM_Walk, "Walk.txt");

    load_animation(ANIM_Jump_Start, "Jump_Start.txt");

    load_animation(ANIM_Action_Neutral_First, "actions/Neutral_First.txt");
    load_animation(ANIM_Action_Tilt_Down, "actions/Tilt_Down.txt");
    load_animation(ANIM_Action_Tilt_Forward, "actions/Tilt_Forward.txt");
    load_animation(ANIM_Action_Tilt_Up, "actions/Tilt_Up.txt");
}

//============================================================================//

void Sara_Fighter::tick()
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
            mAnimTimeContinuous += std::abs(mVelocity.x) * 0.6f;
            update_pose(ANIM_Walk, mAnimTimeContinuous);
        }

        if (state.move == State::Move::Dash)
        {
            // todo: work out exact dash animation speed
            mAnimTimeContinuous += std::abs(mVelocity.x) * 0.6f;
            update_pose(ANIM_Walk, mAnimTimeContinuous);
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
                play_animation(ANIM_Jump_Start);
                mJumpStartDone = true;
            }
        }
        else mJumpStartDone = false;
    }

    //--------------------------------------------------------//

    this->base_tick_animation();
}
