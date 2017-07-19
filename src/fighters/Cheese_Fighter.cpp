#include <sqee/maths/Functions.hpp>

#include "game/FightSystem.hpp"

#include "fighters/Cheese_Actions.hpp"
#include "fighters/Cheese_Fighter.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Cheese_Fighter::Cheese_Fighter(uint8_t index, FightSystem& system, Controller& controller)
    : Fighter(index, system, controller, "Cheese")
{
    mActions = create_actions(mFightSystem, *this);

    //--------------------------------------------------------//

    mSpheres.push_back({ { -0.137f, 0.f, -0.238f }, 0.3f });
    mSpheres.push_back({ { -0.137f, 0.f, +0.238f }, 0.3f });
    mSpheres.push_back({ { +0.137f, 0.f, -0.238f }, 0.3f });
    mSpheres.push_back({ { +0.137f, 0.f, +0.238f }, 0.3f });
    mSpheres.push_back({ { -0.275f, 0.f, 0.f }, 0.3f });
    mSpheres.push_back({ { +0.275f, 0.f, 0.f }, 0.3f });

    //--------------------------------------------------------//

    for ([[maybe_unused]] const auto& sphere : mSpheres)
        mHurtBlobs.push_back(system.create_damageable_hit_blob(*this));
}

//============================================================================//

void Cheese_Fighter::tick()
{
    this->base_tick_entity();
    this->base_tick_fighter();

    //--------------------------------------------------------//

    if (state.move == State::Move::None)
        mColour = Vec3F(1.f, 0.f, 0.f);

    if (state.move == State::Move::Walking)
        mColour = Vec3F(0.f, 1.f, 0.f);

    if (state.move == State::Move::Dashing)
        mColour = Vec3F(0.3f, 0.3f, 0.6f);

    if (state.move == State::Move::Jumping)
        mColour = Vec3F(0.f, 0.f, 1.f);

    if (state.move == State::Move::Falling)
        mColour = Vec3F(0.1f, 0.1f, 2.f);

    //--------------------------------------------------------//

    const Vec3F position = Vec3F(mCurrentPosition, 0.f);
    const QuatF rotation = QuatF(0.f, 0.f, -0.1f * float(state.direction));

    const Mat4F modelMatrix = maths::transform(position, rotation, Vec3F(1.f));

    for (uint i = 0u; i < mSpheres.size(); ++i)
    {
        mHurtBlobs[i]->sphere.origin = Vec3F(modelMatrix * Vec4F(mSpheres[i].origin, 1.f));
        mHurtBlobs[i]->sphere.radius = mSpheres[i].radius;
    }
}
