import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.play_animation("GrabAttack", 1, true)
    state.disable_actions()
  }

  default_end() {
    fighter.set_next_animation("GrabLoop", 0)
    state.enable_actions()
  }

  execute() {
    default_begin()

    wait_until(23)
    default_end()
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
