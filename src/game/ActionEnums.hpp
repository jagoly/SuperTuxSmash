#pragma once

#include <sqee/core/EnumHelper.hpp>

//============================================================================//

namespace sts {

enum class ActionType : int8_t
{
    None = -1,
    NeutralFirst,
    DashAttack,
    TiltDown,
    TiltForward,
    TiltUp,
    EvadeBack,
    EvadeForward,
    Dodge,
    ProneAttack,
    ProneBack,
    ProneForward,
    ProneStand,
    LedgeClimb,
    SmashDown,
    SmashForward,
    SmashUp,
    AirBack,
    AirDown,
    AirForward,
    AirNeutral,
    AirUp,
    AirDodge,
    LandLight,
    LandHeavy,
    LandAttack,
    LandTumble,
    SpecialDown,
    SpecialForward,
    SpecialNeutral,
    SpecialUp,
};

enum class ActionStatus : int8_t
{
    None = -1,
    Running,
    AllowInterrupt,
    Finished,
    RuntimeError
};

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER
(
    sts::ActionType,
    None,
    NeutralFirst,
    DashAttack,
    TiltDown,
    TiltForward,
    TiltUp,
    EvadeBack,
    EvadeForward,
    Dodge,
    ProneAttack,
    ProneBack,
    ProneForward,
    ProneStand,
    LedgeClimb,
    SmashDown,
    SmashForward,
    SmashUp,
    AirBack,
    AirDown,
    AirForward,
    AirNeutral,
    AirUp,
    AirDodge,
    LandLight,
    LandHeavy,
    LandAttack,
    LandTumble,
    SpecialDown,
    SpecialForward,
    SpecialNeutral,
    SpecialUp
)

SQEE_ENUM_HELPER
(
    sts::ActionStatus,
    None,
    Running,
    AllowInterrupt,
    Finished,
    RuntimeError
)
