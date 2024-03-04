import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Action")
    fighter.play_animation("NeutralGrab", 1, true)
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(30)
    default_end()
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
