#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

/// Diamond shape used for collision calculations.
struct LocalDiamond final
{
    float halfWidth, offsetCross, offsetTop;
    Vec2F normLeftDown, normLeftUp, normRightDown, normRightUp;

    void compute_normals()
    {
        normLeftDown = sq::maths::normalize(Vec2F(-offsetCross, -halfWidth));
        normLeftUp = sq::maths::normalize(Vec2F(-offsetCross, +halfWidth));
        normRightDown = sq::maths::normalize(Vec2F(+offsetCross, -halfWidth));
        normRightUp = sq::maths::normalize(Vec2F(+offsetCross, +halfWidth));
    }

    Vec2F min() const { return { -halfWidth, 0.f }; }
    Vec2F max() const { return { +halfWidth, offsetTop }; }
    Vec2F cross() const { return { 0.f, offsetCross }; }
};

//============================================================================//

/// Result of something trying to move.
struct MoveAttempt final
{
    bool collideFloor = false;
    bool collideWall = false;
    bool collideCeiling = false;
    bool collideCorner = false;
    bool onPlatform = false;

    // non-zero if platform edge reached
    int8_t edge = 0;

    Vec2F result = Vec2F();
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::LocalDiamond)
