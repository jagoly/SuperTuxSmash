import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  disable_actions() {
    _actions = false
  }
  enable_actions() {
    _actions = true
  }

  // called when the victim mashes buttons
  subtract_mash_time() {
    _grabTime = _grabTime - 8
  }

  enter() {
    vars.edgeStop = "Always"
    vars.moveMobility = 0.0
    _actions = true
    // https://www.ssbwiki.com/Grab#Grab_time
    _grabTime = 90 + (1.7 * vars.victim.variables.damage).floor
  }

  update() {
    _grabTime = _grabTime - 1

    // fall
    if (base_update_ground()) return "GrabFall"

    // disabled during attacks and throws
    if (!_actions) return

    // free
    if (_grabTime <= 0 || ctrl.input.pressJump) return "GrabFree"

    // attack
    for (frame in ctrl.history) {
      if (frame.pressAttack) return "GrabAttack"
    }

    // throws
    for (frame in ctrl.history) {
      // prefer X
      if (frame.intX.abs >= frame.intY.abs) {
        if (frame.relModX == 1) return "ThrowForward"
        if (frame.relModX == -1) return "ThrowBack"
      }
      else if (frame.modY == 1) return "ThrowUp" // gross
      else if (frame.modY == -1) return "ThrowDown"
    }
  }

  exit() {
    // will be null if we finished a throw
    if (vars.victim) {
      vars.victim.start_action("GrabbedFree")
      vars.victim = null
    }
  }
}
