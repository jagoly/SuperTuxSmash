import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
    //vars.edgeStop = "Input"
    vars.moveMobility = 0.0
    //vars.velocity.x = 0.0
  }

  update() {
    var r // return value

    // fall
    if (base_update_ground()) return "MiscFall"

    // dodges
    if (r = lib.check_GroundDodges(ctrl.input)) return r

    // grab
    if (ctrl.input.pressGrab) return "NeutralGrab"

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

    // walk off the edge
    if (ctrl.input.relIntX >= 3) return "Walk"

    // no longer on an edge
    if (vars.edge != vars.facing) return "MiscNeutral"
  }

  exit() {}
}
