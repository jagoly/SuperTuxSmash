#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

struct alignas(16) CameraBlock
{
    Mat4F viewMat;    // 64
    Mat4F projMat;    // 64
    Mat4F invViewMat; // 64
    Mat4F invProjMat; // 64
    Vec3F position;   // 16
    Vec3F direction;  // 16
};

//============================================================================//

struct alignas(16) StaticBlock
{
    Mat4F  matrix;    // 64
    Mat34F normMat;   // 48
};

//============================================================================//

struct alignas(16) SkellyBlock
{
    Mat4F  matrix;    // 64
    Mat34F normMat;   // 48
    Mat34F bones[80]; // 3840
};

//============================================================================//

struct alignas(16) EffectBlock
{
    Mat4F  matrix;    // 64
    Mat34F normMat;   // 48
    Vec4F  params[8]; // 128
    Mat34F bones[8];  // 384
};

//============================================================================//

} // namespace sts
