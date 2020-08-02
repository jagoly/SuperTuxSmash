#pragma once

#include <sqee/core/EnumHelper.hpp>

//============================================================================//

namespace sts {

enum class FighterEnum : int8_t
{
    Null = -1,
    Sara,
    Tux,
    Mario
};

enum class StageEnum : int8_t
{
    Null = -1,
    TestZone
};

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::FighterEnum, Null, Sara, Tux, Mario)
SQEE_ENUM_HELPER(sts::StageEnum, Null, TestZone)
