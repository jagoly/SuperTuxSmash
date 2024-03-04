import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    if (vars.onGround) {
      fighter.change_state("Action")
      fighter.play_animation("GrabFree", 2, true)
      state.doFall = "MiscFall"
    } else {
      fighter.change_state("ActionAir")
      fighter.play_animation("GrabFree", 2, true)
      state.doLand = "MiscLand"
      vars.moveMobility = 0.0
    }
  }

  default_end() {
    if (vars.onGround) {
      fighter.change_state("Neutral")
      fighter.set_next_animation("NeutralLoop", 0)
    } else {
      fighter.change_state("Fall")
      fighter.set_next_animation("FallLoop", 4)
    }
  }

  execute() {
    default_begin()

    wait_until(30)
    default_end()
  }
}
