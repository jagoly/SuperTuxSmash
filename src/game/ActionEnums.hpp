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
    LandTumble,
    LandAirBack,
    LandAirDown,
    LandAirForward,
    LandAirNeutral,
    LandAirUp,
    SpecialDown,
    SpecialForward,
    SpecialNeutral,
    SpecialUp,
    HopBack,
    HopForward,
    JumpBack,
    JumpForward,
    AirHopBack,
    AirHopForward,
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
    LandTumble,
    LandAirBack,
    LandAirDown,
    LandAirForward,
    LandAirNeutral,
    LandAirUp,
    SpecialDown,
    SpecialForward,
    SpecialNeutral,
    SpecialUp,
    HopBack,
    HopForward,
    JumpBack,
    JumpForward,
    AirHopBack,
    AirHopForward
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
