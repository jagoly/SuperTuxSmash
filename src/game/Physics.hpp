#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

/// Diamond shape used for collision calculations.
struct Diamond final
{
    Vec2F cross, min, max;

    Vec2F left() const { return { min.x, cross.y }; }
    Vec2F right() const { return { max.x, cross.y }; }
    Vec2F bottom() const { return { cross.x, min.y }; }
    Vec2F top() const { return { cross.x, max.y }; }

    float widthLeft() const { return cross.x - min.x; }
    float widthRight() const { return max.x - cross.x; }
    float height() const { return max.y - min.y; }
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

WRENPLUS_TRAITS_HEADER(sts::Diamond)
