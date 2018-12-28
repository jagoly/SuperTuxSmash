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
    Tux
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

SQEE_ENUM_HELPER(sts::FighterEnum, Null, Sara, Tux)
SQEE_ENUM_HELPER(sts::StageEnum, Null, TestZone)
