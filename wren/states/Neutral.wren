import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
    vars.extraJumps = attrs.extraJumps
    vars.edgeStop = "Always"
    vars.applyGravity = true
    vars.applyFriction = true
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

    // grab
    for (frame in ctrl.history) {
      if (frame.pressGrab) return "NeutralGrab"
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

    // dash
    for (frame in ctrl.history) {
      if (r = lib.check_DashStart(frame)) return r
    }

    // crouch
    if (ctrl.input.intY == -4) return "CrouchOn"

    // walk
    if (ctrl.input.relIntX >= 1) return "Walk"

    // turn
    for (frame in ctrl.history) {
      if (frame.relIntX >= 1) break
      if (frame.relIntX <= -1) return "Turn"
    }

    // vertigo
    if (vars.edge == vars.facing) return "Vertigo"
  }

  exit() {}
}
