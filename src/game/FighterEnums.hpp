#pragma once

#include "setup.hpp"

//============================================================================//

namespace sts {

/// Describes the way that an animation should be updated and applied.
enum class FighterAnimMode : uint8_t
{
    Basic,      ///< non-looping animation
    Loop,       ///< looping animation
    WalkLoop,   ///< looping, update using velocity and walkAnimStride
    DashLoop,   ///< looping, update using velocity and dashAnimStride
    Motion,     ///< non-looping, extract root motion to object
    Turn,       ///< non-looping, extract facing change to object
    MotionTurn, ///< non-looping, Motion + Turn
};

/// Describes when a fighter should stop at the edge of a platform.
enum class FighterEdgeStopMode : uint8_t
{
    Never,  ///< never stop at an edge
    Always, ///< always stop at an edge
    Input,  ///< stop depending on input
};

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER
(
    sts::FighterAnimMode,
    Basic,
    Loop,
    WalkLoop,
    DashLoop,
    Motion,
    Turn,
    MotionTurn
)

SQEE_ENUM_HELPER
(
    sts::FighterEdgeStopMode,
    Never,
    Always,
    Input
)
