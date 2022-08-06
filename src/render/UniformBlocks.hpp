#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

struct CameraBlock
{
    alignas(16) Mat4F viewMat;
    alignas(16) Mat4F projMat;
    alignas(16) Mat4F invViewMat;
    alignas(16) Mat4F invProjMat;
    alignas(16) Mat4F projViewMat;
};

//============================================================================//

struct EnvironmentBlock
{
    alignas(16) Vec3F lightColour;
    alignas(16) Vec3F lightDirection;
    alignas(16) Mat4F viewMatrix;
    alignas(16) Mat4F projViewMatrix;
};

//============================================================================//

} // namespace sts
