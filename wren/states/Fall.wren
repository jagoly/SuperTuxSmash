import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
    vars.applyGravity = true
    vars.applyFriction = true
    vars.moveMobility = attrs.airMobility
    vars.moveSpeed = attrs.airSpeed
  }

  update() {
    var r // return value

    // land
    if (base_update_air()) return "MiscLand"

    // ledge catch
    if (fighter.attempt_ledge_catch()) return "LedgeCatch"

    // specials
    for (frame in ctrl.history) {
      if (r = lib.check_AirSpecials(frame)) return r
    }

    // dodge
    for (frame in ctrl.history) {
      if (frame.pressShield) return "AirDodge"
    }

    // attacks
    for (frame in ctrl.history) {
      if (r = lib.check_AirAttacks(frame)) return r
    }

    // air hops
    for (frame in ctrl.history) {
      if (r = lib.check_AirHops(frame)) return r
    }

    // fast fall
    if (!vars.fastFall && vars.velocity.y < 0.0) {
      for (frame in ctrl.history) {
        if (frame.mashY == -1) return "FastFall"
      }
    }
  }

  exit() {
    vars.fastFall = false
  }
}
