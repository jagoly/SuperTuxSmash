import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // call to disable evade actions
  disable_reverse_evade() {
    _reverseEvade = false
  }

  // allow movement on the next frame
  enable_movement() {
    vars.moveMobility = attrs.traction * 4.0
    vars.moveSpeed = attrs.dashSpeed
  }

  // allow changing to Neutral
  enable_stop() {
    _allowStop = true
  }

  enter() {
    vars.edgeStop = "Always"
    vars.moveMobility = 0.0
    _reverseEvade = true
    _allowStop = false
  }

  update() {
    var r // return value

    // fall
    if (base_update_ground()) return "MiscFall"

    // dodges
    if (_reverseEvade) {
      if (r = lib.check_GroundDodgesRv(ctrl.input)) return r
    }

    // specials
    if (r = lib.check_GroundSpecials(ctrl.input)) return r

    // smashes
    if (r = lib.check_GroundSmashes(ctrl.input)) return r

    // attacks
    if (r = lib.check_GroundAttacks(ctrl.input)) return r

    // jump
    if (ctrl.input.pressJump) return "JumpSquat"

    // stop
    if (_allowStop && ctrl.input.relIntX <= 2) {
      return "BrakeTurnStop"
    }
  }

  exit() {}
}
