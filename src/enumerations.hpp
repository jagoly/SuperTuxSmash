#pragma once

#include <sqee/setup.hpp>
#include <sqee/macros.hpp>

namespace sts {

//============================================================================//

enum class GameMode : int8_t
{
    Null = -1,
    Standard,
    Editor
};

//============================================================================//

enum class FighterEnum : int8_t
{
    Null = -1,
    Sara,
    Tux,
    Count
};

enum class StageEnum : int8_t
{
    Null = -1,
    TestZone,
    Count
};

//============================================================================//

SQEE_ENUM_TO_STRING(FighterEnum, Null, Sara, Tux, Count);
SQEE_ENUM_TO_STRING(StageEnum, Null, TestZone, Count)

//============================================================================//

} // namespace sts
