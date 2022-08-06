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
        normLeftDown = maths::normalize(Vec2F(-offsetCross, -halfWidth));
        normLeftUp = maths::normalize(Vec2F(-offsetCross, +halfWidth));
        normRightDown = maths::normalize(Vec2F(+offsetCross, -halfWidth));
        normRightUp = maths::normalize(Vec2F(+offsetCross, +halfWidth));
    }

    Vec2F min() const { return { -halfWidth, 0.f }; }
    Vec2F max() const { return { +halfWidth, offsetTop }; }
    Vec2F cross() const { return { 0.f, offsetCross }; }
};

//============================================================================//

/// Result of a fighter trying to move.
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

/// Result of an article trying to move.
struct MoveAttemptSphere final
{
    Vec2F newPosition = Vec2F();
    Vec2F newVelocity = Vec2F();

    bool bounced = false;
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::LocalDiamond)
