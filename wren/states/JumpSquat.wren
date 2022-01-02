import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // set vertical velocity for hop or jump
  second_last_frame() {
    if (_jumpHeld && ctrl.input.holdJump) {
      _shortHop = false
      lib.assign_jump_velocity_y(attrs.jumpHeight)
    } else {
      _shortHop = true
      lib.assign_jump_velocity_y(attrs.hopHeight)
    }
  }

  // set horizontal velocity and return action name
  last_frame() {
    lib.assign_jump_velocity_x()
    if (_shortHop) {
      return ctrl.input.relIntX >= 0 ? "HopForward" : "HopBack"
    } else {
      return ctrl.input.relIntX >= 0 ? "JumpForward" : "JumpBack"
    }
  }

  enter() {
    vars.edgeStop = "Always"
    vars.moveMobility = 0.0
    _jumpHeld = ctrl.input.holdJump
  }

  update() {
    if (!ctrl.input.holdJump) _jumpHeld = false
  }

  exit() {}
}
