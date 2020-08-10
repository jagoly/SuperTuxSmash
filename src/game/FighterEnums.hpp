#pragma once

#include <sqee/core/EnumHelper.hpp>

//============================================================================//

namespace sts {

// todo: comments for fighter states

/// The general state of a fighter which changes how they are updated.
enum class FighterState : uint8_t
{
    Neutral,
    Walking,
    Dashing,
    Brake,
    Crouch,
    PreJump,
    Prone,
    Shield,
    JumpFall,
    TumbleFall,
    HitStun,
    TumbleStun,
    Helpless,
    LedgeHang,
    Charge,
    Action,
    AirAction,
    Freeze,
    EditorPreview
};

/// A command for the fighter to act on, generated by the player.
enum class FighterCommand : uint8_t
{
    Shield,        ///< used to perform evades/dodges
    Jump,          ///< used to jump and airhop
    MashDown,      ///< used to drop through platforms and fastfall
    MashUp,        ///< used for getting up from prone
    MashLeft,      ///< used to start dashing
    MashRight,     ///< used to start dashing
    TurnLeft,      ///< used to change facing
    TurnRight,     ///< used to change facing
    SmashDown,     ///< begin charging a down smash
    SmashUp,       ///< begin charging an up smash
    SmashLeft,     ///< begin charging a forward smash
    SmashRight,    ///< begin charging a forward smash
    AttackDown,    ///< perform dtilt, dair or dash attack
    AttackUp,      ///< perform utilt, uair or dash attack
    AttackLeft,    ///< perform ftilt, fair, bair or dash attack
    AttackRight,   ///< perform ftilt, fair, bair or dash attack
    AttackNeutral, ///< perform neutral, nair or dash attack
};

/// Describes the way that an animation should be updated and applied.
enum class FighterAnimMode : uint8_t
{
    Standard,    ///< non-looping, without root motion
    Looping,     ///< looping, without root motion
    WalkCycle,   ///< looping, update using velocity and anim_walk_stride
    DashCycle,   ///< looping, update using velocity and anim_dash_stride
    ApplyMotion, ///< non-looping, extract root motion to object
};

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER
(
    sts::FighterState,
    Neutral,
    Walking,
    Dashing,
    Brake,
    Crouch,
    PreJump,
    Prone,
    Shield,
    JumpFall,
    TumbleFall,
    HitStun,
    TumbleStun,
    Helpless,
    LedgeHang,
    Charge,
    Action,
    AirAction,
    Freeze,
    EditorPreview
)

SQEE_ENUM_HELPER
(
    sts::FighterCommand,
    Shield,
    Jump,
    MashDown,
    MashUp,
    MashLeft,
    MashRight,
    TurnLeft,
    TurnRight,
    SmashDown,
    SmashUp,
    SmashLeft,
    SmashRight,
    AttackDown,
    AttackUp,
    AttackLeft,
    AttackRight,
    AttackNeutral
)

SQEE_ENUM_HELPER
(
    sts::FighterAnimMode,
    Standard,
    Looping,
    WalkCycle,
    DashCycle,
    ApplyMotion
)
