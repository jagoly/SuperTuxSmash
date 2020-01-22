#pragma once

#include <sqee/setup.hpp>
#include <sqee/macros.hpp>

namespace sts {

//============================================================================//

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

//============================================================================//

class Fighter;
enum class ActionType : int8_t;

namespace message {

struct fighter_action_finished
{
    Fighter& fighter;
    ActionType type;
};

} // namespace message

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::FighterEnum, Null, Sara, Tux, Mario)
SQEE_ENUM_HELPER(sts::StageEnum, Null, TestZone)
