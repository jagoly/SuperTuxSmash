import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Action")
    state.doFall = "MiscFall"
    fighter.play_animation("Rebound", 2, true)
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(vars.reboundTime)
    default_end()
  }
}
