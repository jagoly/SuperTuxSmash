import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // disable reversing evade actions
  disable_reverse_evade() {
    _reverseEvade = false
  }

  enter() {
    vars.edgeStop = "Input"
    vars.moveMobility = 0.0
    _reverseEvade = true
  }

  update() {
    var r // return value

    // fall
    if (base_update_ground()) return "MiscFall"

    // dodges
    if (_reverseEvade) {
      if (r = lib.check_GroundDodgesRv(ctrl.input)) return r
    } else {
      if (r = lib.check_GroundDodges(ctrl.input)) return r
    }

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
    for (frame in ctrl.history) {
      if (r = lib.check_DashStart(frame)) return r
    }

    // crouch
    if (ctrl.input.intY == -4) return "CrouchOn"
  }

  exit() {}
}
