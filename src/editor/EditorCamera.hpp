#pragma once

#include "setup.hpp"

#include "render/Camera.hpp" // IWYU pragma: export

namespace sts {

//============================================================================//

class EditorCamera final : public Camera
{
public: //====================================================//

    using Camera::Camera;

    void update_from_world(const World& world) override;

    void update_from_controller(const Controller& controller) override;

    void integrate(float blend) override;

private: //===================================================//

    float mYaw = 0.f;
    float mPitch = 0.f;
    float mZoom = 4.f;
    Vec2F mCentre = { 0.f, 1.f };
};

//============================================================================//

} // namespace sts
