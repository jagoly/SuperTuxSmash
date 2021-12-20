import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
    vars.extraJumps = attrs.extraJumps
    vars.edgeStop = "Never"
    vars.applyGravity = true
    vars.applyFriction = true
    vars.flinch = true
    vars.moveMobility = 0.0
  }

  update() {

    // fall
    if (base_update_ground()) return "MiscTumble"

    // attack
    for (frame in ctrl.history) {
      if (frame.pressAttack) return "ProneAttack"
    }

    // evade or stand
    for (frame in ctrl.history) {
      if (frame.intX.abs > frame.intY) {
        if (frame.relIntX >= 2) return "ProneForward"
        if (frame.relIntX <= -2) return "ProneBack"
      }
      if (frame.intY >= 2) return "ProneStand"
    }
  }

  exit() {
    vars.flinch = false
  }
}
