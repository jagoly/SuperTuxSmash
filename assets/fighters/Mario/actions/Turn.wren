import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.reverse_facing_animated(true)
    fighter.change_state("Turn")
    fighter.play_animation("Turn", 2, true)
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(6)
    state.disable_reverse_evade()

    wait_until(10)
    default_end()
  }
}
