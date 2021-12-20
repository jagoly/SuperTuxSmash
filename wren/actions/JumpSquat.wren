import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("JumpSquat")
    fighter.play_animation("JumpSquat", 1, true)
  }

  execute() {
    default_begin()

    wait_until(5)
    state.second_last_frame()

    wait_until(6)
    return state.last_frame()
  }
}
