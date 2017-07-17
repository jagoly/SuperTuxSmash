#include "game/FightSystem.hpp"

#include "fighters/Cheese_Actions.hpp"
#include "fighters/Cheese_Fighter.hpp"

using namespace sts;

//============================================================================//

Cheese_Fighter::Cheese_Fighter(FightSystem& system, Controller& controller)
    : Fighter(system, controller, "Cheese")
{
    mActions = create_actions(mFightSystem, *this);

    //--------------------------------------------------------//

    mSpheres.push_back({ { -0.15f, +0.18f, +0.1f }, 0.35f });
    mSpheres.push_back({ { -0.15f, -0.18f, +0.1f }, 0.35f });
    mSpheres.push_back({ { +0.15f, +0.18f, -0.1f }, 0.35f });
    mSpheres.push_back({ { +0.15f, -0.18f, -0.1f }, 0.35f });

    //--------------------------------------------------------//

    for ([[maybe_unused]] const auto& sphere : mSpheres)
        mHurtBlobs.push_back(system.create_hit_blob(HitBlob::Type::Damageable, *this));
}

//============================================================================//

void Cheese_Fighter::tick()
{
    this->base_tick_entity();
    this->base_tick_fighter();

    //--------------------------------------------------------//

    for (size_t i = 0u; i < mSpheres.size(); ++i)
    {
        auto& blobSphere = (mHurtBlobs[i]->sphere = mSpheres[i]);
        blobSphere.origin.x *= float(state.direction);
        blobSphere.origin.x += mCurrentPosition.x;
        blobSphere.origin.z += mCurrentPosition.y;
    }

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
}
