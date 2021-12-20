import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
    vars.edgeStop = "Always"
    vars.moveMobility = 0.0
  }

  update() {

    // fall
    if (base_update_ground()) return "MiscFall"

    // dodges
    for (frame in ctrl.history) {
      if (frame.intX.abs > -frame.intY || frame.mashY >= 0) {
        if (frame.relMashX == 1) return "EvadeForward"
        if (frame.relMashX == -1) return "EvadeBack"
      }
      if (frame.mashY == -1) return "Dodge"
    }

    // jump
    for (frame in ctrl.history) {
      if (frame.pressJump) return "JumpSquat"
    }

    // shield off
    if (!ctrl.input.holdShield) return "ShieldOff"
  }

  exit() {}
}
