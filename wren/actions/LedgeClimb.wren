import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    state.disable_exit_drop()
    fighter.change_state("Action")
    fighter.play_animation("LedgeClimb", 1, true)
    state.doFall = "MiscFall"
    vars.intangible = true
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(30)
    vars.intangible = false

    wait_until(35)
    default_end()
  }

  cancel() {
    vars.intangible = false
  }
}
