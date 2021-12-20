import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("Action")
    fighter.play_animation("TiltUp", 1, true)
    state.doFall = "MiscFall"
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(32)
    default_end()
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
