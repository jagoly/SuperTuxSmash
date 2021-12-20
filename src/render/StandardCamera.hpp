#pragma once

#include "setup.hpp"

#include "render/Camera.hpp" // IWYU pragma: export

namespace sts {

//============================================================================//

class StandardCamera final : public Camera
{
public: //====================================================//

    using Camera::Camera;

    void update_from_world(const FightWorld& world);

    void intergrate(float blend) override;

private: //===================================================//

    std::array<MinMax<Vec2F>, 16> mViewHistory;

    MinMax<Vec2F> mPreviousView, mCurrentView;
    MinMax<Vec2F> mPreviousBounds, mCurrentBounds;
};

//============================================================================//

} // namespace sts
