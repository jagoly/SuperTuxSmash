#pragma once

#include <sqee/core/EnumHelper.hpp>

//============================================================================//

namespace sts {

enum class ActionType : int8_t
{
    None = -1,
    NeutralFirst,
    NeutralSecond,
    NeutralThird,
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
    ChargeDown,
    ChargeForward,
    ChargeUp,
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
    ShieldOn,
    ShieldOff,
    HopBack,
    HopForward,
    JumpBack,
    JumpForward,
    AirHopBack,
    AirHopForward,
    DashStart,
    DashBrake,
};

enum class ActionStatus : int8_t
{
    None = -1,
    Running,
    AllowInterrupt,
    Finished,
    RuntimeError
};

enum class ActionFlag : uint8_t
{
    // set from c++
    AttackHeld  = 1 << 0,
    HitCollide  = 1 << 1,

    // set from wren
    AllowNext   = 1 << 2,
    AutoJab     = 1 << 3,
};

struct ActionTraits
{
    const bool needInterrupt, exclusive, aerial;

    static constexpr ActionTraits get(ActionType type)
    {
        switch (type) {

        case ActionType::None           : return { false, false, false, };
        case ActionType::NeutralFirst   : return { true,  true,  false, };
        case ActionType::NeutralSecond  : return { true,  true,  false, };
        case ActionType::NeutralThird   : return { true,  true,  false, };
        case ActionType::DashAttack     : return { true,  true,  false, };
        case ActionType::TiltDown       : return { true,  true,  false, };
        case ActionType::TiltForward    : return { true,  true,  false, };
        case ActionType::TiltUp         : return { true,  true,  false, };
        case ActionType::EvadeBack      : return { true,  true,  false, };
        case ActionType::EvadeForward   : return { true,  true,  false, };
        case ActionType::Dodge          : return { true,  true,  false, };
        case ActionType::ProneAttack    : return { true,  true,  false, };
        case ActionType::ProneBack      : return { true,  true,  false, };
        case ActionType::ProneForward   : return { true,  true,  false, };
        case ActionType::ProneStand     : return { true,  true,  false, };
        case ActionType::LedgeClimb     : return { true,  true,  false, };
        case ActionType::ChargeDown     : return { false, true,  false, };
        case ActionType::ChargeForward  : return { false, true,  false, };
        case ActionType::ChargeUp       : return { false, true,  false, };
        case ActionType::SmashDown      : return { true,  true,  false, };
        case ActionType::SmashForward   : return { true,  true,  false, };
        case ActionType::SmashUp        : return { true,  true,  false, };
        case ActionType::AirBack        : return { true,  true,  true,  };
        case ActionType::AirDown        : return { true,  true,  true,  };
        case ActionType::AirForward     : return { true,  true,  true,  };
        case ActionType::AirNeutral     : return { true,  true,  true,  };
        case ActionType::AirUp          : return { true,  true,  true,  };
        case ActionType::AirDodge       : return { true,  true,  true,  };
        case ActionType::LandLight      : return { true,  true,  false, };
        case ActionType::LandHeavy      : return { true,  true,  false, };
        case ActionType::LandTumble     : return { true,  true,  false, };
        case ActionType::LandAirBack    : return { true,  true,  false, };
        case ActionType::LandAirDown    : return { true,  true,  false, };
        case ActionType::LandAirForward : return { true,  true,  false, };
        case ActionType::LandAirNeutral : return { true,  true,  false, };
        case ActionType::LandAirUp      : return { true,  true,  false, };
        case ActionType::SpecialDown    : return { true,  true,  false, };
        case ActionType::SpecialForward : return { true,  true,  false, };
        case ActionType::SpecialNeutral : return { true,  true,  false, };
        case ActionType::SpecialUp      : return { true,  true,  false, };
        case ActionType::ShieldOn       : return { false, false, false, };
        case ActionType::ShieldOff      : return { true,  true,  false, };
        case ActionType::HopBack        : return { false, false, true,  };
        case ActionType::HopForward     : return { false, false, true,  };
        case ActionType::JumpBack       : return { false, false, true,  };
        case ActionType::JumpForward    : return { false, false, true,  };
        case ActionType::AirHopBack     : return { false, false, true,  };
        case ActionType::AirHopForward  : return { false, false, true,  };
        case ActionType::DashStart      : return { false, false, false, };
        case ActionType::DashBrake      : return { false, false, false, };

        } // switch

        return { false, false, false, };
    }
};

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER
(
    sts::ActionType,
    None,
    NeutralFirst,
    NeutralSecond,
    NeutralThird,
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
    ChargeDown,
    ChargeForward,
    ChargeUp,
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
    ShieldOn,
    ShieldOff,
    HopBack,
    HopForward,
    JumpBack,
    JumpForward,
    AirHopBack,
    AirHopForward,
    DashStart,
    DashBrake
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
