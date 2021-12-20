import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Brake")
    fighter.play_animation("Brake", 2, true)
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(10)
    state.disable_allow_turn()

    wait_until(13)
    default_end()
  }
}
