import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("Action")
    fighter.play_animation("LedgeEvade", 1, true)
    state.doFall = null
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

    wait_until(50)
    default_end()
  }

  cancel() {
    vars.intangible = false
  }
}
