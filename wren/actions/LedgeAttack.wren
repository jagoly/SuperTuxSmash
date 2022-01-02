import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("Action")
    fighter.play_animation("LedgeAttack", 1, true)
    state.doFall = null
    vars.intangible = true
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(20)
    vars.intangible = false

    wait_until(55)
    default_end()
  }

  cancel() {
    action.disable_hitblobs(true)
    vars.intangible = false
  }
}
