import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  start_throw(animation) {
    fighter.play_animation(animation, 2, true)
  }

  finish_throw() {
    fighter.change_state("TumbleStun")
    fighter.play_animation("HurtMiddleTumble", 4, true)
    fighter.set_next_animation("TumbleLoop", 0)
  }

  enter() {
    vars.edgeStop = "Never"
    vars.moveMobility = 0.0
    vars.applyGravity = false
    vars.flinch = true
    _onGround = vars.onGround
    _numMashes = 0

    if (vars.facing == vars.bully.variables.facing) {
      fighter.reverse_facing_animated(true)
    }
  }

  update() {

    if (_onGround) {
      if (base_update_ground()) {
        fighter.play_animation("GrabbedLoopHigh", 2, false)
        _onGround = false
      }
    } else {
      if (base_update_air()) {
        fighter.play_animation("GrabbedLoopLow", 2, false)
        _onGround = true
      }
    }

    if (ctrl.input.mashX != 0 || ctrl.input.mashY != 0) _numMashes = _numMashes + 1
    if (ctrl.input.pressAttack) _numMashes = _numMashes + 1
    if (ctrl.input.pressSpecial) _numMashes = _numMashes + 1
    if (ctrl.input.pressJump) _numMashes = _numMashes + 1
    if (ctrl.input.pressShield) _numMashes = _numMashes + 1
    if (ctrl.input.pressGrab) _numMashes = _numMashes + 1

    // limit to one mash per frame, but allow them to be buffered
    if (_numMashes > 0) {
      vars.bully.state.script.subtract_mash_time()
      _numMashes = _numMashes - 1
    }
  }

  exit() {
    vars.bully = null
    vars.applyGravity = true
    vars.flinch = false
  }
}
