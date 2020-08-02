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

    struct MinMax { Vec2F min, max; };

    std::array<MinMax, 16u> mViewHistory;

    MinMax mPreviousView, mCurrentView;
    MinMax mPreviousBounds, mCurrentBounds;
};

//============================================================================//

} // namespace sts
