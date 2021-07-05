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
    alignas(16) Vec3F position;
    alignas(16) Vec3F direction;
};

//============================================================================//

struct EnvironmentBlock
{
    alignas(16) Vec3F lightColour;
    alignas(16) Vec3F lightDirection;
    alignas(16) Mat4F lightMatrix;
};

//============================================================================//

struct StaticBlock
{
    alignas(16) Mat4F  matrix;
    alignas(16) Mat34F normMat;
};

//============================================================================//

struct SkellyBlock
{
    alignas(16) Mat4F  matrix;
    alignas(16) Mat34F normMat;
    alignas(16) Mat34F bones[80];
};

//============================================================================//

struct EffectBlock
{
    alignas(16) Mat4F  matrix;
    alignas(16) Mat34F normMat;
    alignas(16) Vec4F  params[8];
    alignas(16) Mat34F bones[8];
};

//============================================================================//

} // namespace sts
