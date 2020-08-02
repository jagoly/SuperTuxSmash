#pragma once

#include <sqee/core/EnumHelper.hpp>

//============================================================================//

namespace sts {

enum class ActionType : int8_t
{
    None = -1,
    NeutralFirst,
    TiltDown,
    TiltForward,
    TiltUp,
    AirBack,
    AirDown,
    AirForward,
    AirNeutral,
    AirUp,
    DashAttack,
    SmashDown,
    SmashForward,
    SmashUp,
    SpecialDown,
    SpecialForward,
    SpecialNeutral,
    SpecialUp,
    EvadeBack,
    EvadeForward,
    Dodge,
    AirDodge
};

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER
(
    sts::ActionType,
    None,
    NeutralFirst,
    TiltDown,
    TiltForward,
    TiltUp,
    AirBack,
    AirDown,
    AirForward,
    AirNeutral,
    AirUp,
    DashAttack,
    SmashDown,
    SmashForward,
    SmashUp,
    SpecialDown,
    SpecialForward,
    SpecialNeutral,
    SpecialUp,
    EvadeBack,
    EvadeForward,
    Dodge,
    AirDodge
)
