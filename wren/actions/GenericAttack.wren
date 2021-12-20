import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin(animation) {
    ctrl.clear_history()
    fighter.change_state("Action")
    fighter.play_animation(animation, 1, true)
    state.doFall = "MiscFall"
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
