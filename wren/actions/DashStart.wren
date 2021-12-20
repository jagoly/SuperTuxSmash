import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("DashStart")
    fighter.play_animation("DashStart", 1, true)
    state.enable_movement()
    state.disable_reverse_evade()
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(6)
    state.disable_early_actions()

    wait_until(10)
    state.enable_late_actions()

    wait_until(14)
    default_end()
  }
}
