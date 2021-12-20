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

    // land
    if (base_update_air()) return "LandHelpless"

    // ledge catch
    if (fighter.attempt_ledge_catch()) return "LedgeCatch"

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
