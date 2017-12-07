#pragma once

#include <sqee/maths/Volumes.hpp>

#include "main/Options.hpp"

//============================================================================//

namespace sts {

struct SceneData
{
    struct { Vec2F min, max; } view;  ///< active objects boundary
    struct { Vec2F min, max; } inner; ///< inner stage boundary area
    struct { Vec2F min, max; } outer; ///< outer stage boundary area
};

} // namespace sts
