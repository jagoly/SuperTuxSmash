import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  // -1 = Down, 0 = Forward, 1 = Up
  angle { _angle }
  angle=(value) { _angle = value }

  // set false to prevent changing angle
  allowAngle { _allowAngle }
  allowAngle=(value) { _allowAngle = value }

  default_begin() {
    fighter.change_state("Action")
    allowAngle = (angle = ctrl.input.intY.sign) == 0
    if (angle == 1) {
      fighter.play_animation("SmashForwardUp", 1, true)
    } else if (angle == -1) {
      fighter.play_animation("SmashForwardDown", 1, true)
    } else {
      fighter.play_animation("SmashForward", 1, true)
    }
    state.doFall = "MiscFall"
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(2)
    allowAngle = false

    wait_until(36)
    default_end()
  }

  update() {
    if (allowAngle) {
      allowAngle = (angle = ctrl.input.intY.sign) == 0
      if (angle == 1) {
        fighter.play_animation("SmashForwardUp", 0, false)
      } else if (angle == -1) {
        fighter.play_animation("SmashForwardDown", 0, false)
      }
    }
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
