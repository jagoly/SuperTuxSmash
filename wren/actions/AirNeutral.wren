import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("ActionAir")
    fighter.play_animation("AirNeutral", 1, true)
    state.doLand = "MiscLand"
  }

  default_end() {
    fighter.change_state("Fall")
    fighter.set_next_animation("FallLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(2)
    state.doLand = "LandAirNeutral"

    wait_until(20)
    state.doLand = "MiscLand"

    wait_until(32)
    default_end()
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
