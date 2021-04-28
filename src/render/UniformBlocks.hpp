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

struct LightBlock
{
    alignas(16) Vec3F ambiColour;
    alignas(16) Vec3F skyColour;
    alignas(16) Vec3F skyDirection;
    alignas(16) Mat4F skyMatrix;
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
