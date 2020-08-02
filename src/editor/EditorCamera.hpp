#pragma once

#include "setup.hpp"

#include "render/Camera.hpp" // IWYU pragma: export

namespace sts {

//============================================================================//

class EditorCamera final : public Camera
{
public: //====================================================//

    using Camera::Camera;

    void update_from_scroll(float delta);

    void update_from_mouse(bool left, bool right, Vec2F position);

    void intergrate(float blend) override;

private: //===================================================//

    Vec2F mPrevMousePosition;

    float mYaw = 0.f;
    float mPitch = 0.f;
    float mZoom = 4.f;
    Vec2F mCentre = { 0.f, 1.f };
};

//============================================================================//

} // namespace sts
