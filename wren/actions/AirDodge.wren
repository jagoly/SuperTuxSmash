import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("ActionAir")
    fighter.play_animation("AirDodge", 1, true)
    state.doLand = "LandHeavy"
  }

  default_end() {
    fighter.change_state("Fall")
    fighter.set_next_animation("FallLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(4)
    vars.intangible = true

    wait_until(32)
    vars.intangible = false

    wait_until(48)
    default_end()
  }

  cancel() {
    vars.intangible = false
  }
}
