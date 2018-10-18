#pragma once

#include <sqee/misc/Builtins.hpp>
#include <sqee/maths/Builtins.hpp>

#ifndef SQEE_MSVC
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wpadded\"")
#endif

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

static_assert(sizeof(CameraBlock) == 288);

//============================================================================//

template <size_t NumBones>
struct alignas(16) CharacterBlock
{
    Mat4F  matrix;  // 64
    Mat34F normMat; // 48
    Array<Mat34F, NumBones> bones;
};

//static_assert(sizeof(CharacterBlock<80>) == 3952);

//============================================================================//

#ifndef SQEE_MSVC
_Pragma("GCC diagnostic pop")
#endif
