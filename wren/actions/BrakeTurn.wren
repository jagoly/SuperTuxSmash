import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.reverse_facing_auto()
    fighter.change_state("BrakeTurn")
    fighter.play_animation("BrakeTurn", 1, true)
  }

  default_end() {
    fighter.change_state("Dash")
    fighter.play_animation("DashLoop", 0, true)
  }

  execute() {
    default_begin()

    wait_until(6)
    state.disable_reverse_evade()

    wait_until(14)
    state.enable_movement()
    state.enable_stop()

    wait_until(21)
    default_end()
  }
}
