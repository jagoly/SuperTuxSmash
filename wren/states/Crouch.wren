import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
    vars.edgeStop = "Always"
    vars.moveMobility = 0.0
  }

  update() {
    var r // return value

    // fall
    if (base_update_ground()) return "MiscFall"

    // dodges
    for (frame in ctrl.history) {
      if (r = lib.check_GroundDodges(frame)) return r
    }

    // shield
    if (ctrl.input.holdShield) return "ShieldOn"

    // specials
    for (frame in ctrl.history) {
      if (r = lib.check_GroundSpecials(frame)) return r
    }

    // smashes
    for (frame in ctrl.history) {
      if (r = lib.check_GroundSmashes(frame)) return r
    }

    // attacks
    for (frame in ctrl.history) {
      if (r = lib.check_GroundAttacks(frame)) return r
    }

    // jump
    for (frame in ctrl.history) {
      if (frame.pressJump) return "JumpSquat"
    }

    // stop crouching
    if (ctrl.input.intY != -4) return "CrouchOff"

    // platform drop
    if (vars.onPlatform) {
      for (frame in ctrl.history) {
        if (frame.mashY == -1) return "PlatformDrop"
      }
    }
  }

  exit() {}
}
