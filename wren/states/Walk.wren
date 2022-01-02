import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
    vars.edgeStop = "Input"
    vars.moveMobility = attrs.traction * 2.0
    vars.moveSpeed = attrs.walkSpeed
    vars.velocity.x = (vars.velocity.x + vars.moveMobility * vars.facing)
  }

  update() {
    var r // return value

    // fall
    if (base_update_ground()) return "MiscFall"

    // dodges
    if (r = lib.check_GroundDodges(ctrl.input)) return r

    // shield
    if (ctrl.input.holdShield) return "ShieldOn"

    // specials
    if (r = lib.check_GroundSpecials(ctrl.input)) return r

    // smashes
    if (r = lib.check_GroundSmashes(ctrl.input)) return r

    // attacks
    if (r = lib.check_GroundAttacks(ctrl.input)) return r

    // jump
    if (ctrl.input.pressJump) return "JumpSquat"

    // dash
    if (r = lib.check_DashStart(ctrl.input)) return r

    // crouch
    if (ctrl.input.intY == -4) return "CrouchOn"

    // turn
    if (ctrl.input.relIntX <= -1) return "Turn"

    // stop walking
    if (ctrl.input.relIntX == 0) return "MiscNeutral"
  }

  exit() {}
}
