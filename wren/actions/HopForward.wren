import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Fall")
    fighter.play_animation("JumpForward", 0, true)
    fighter.set_next_animation("FallLoop", 0)
  }

  execute() {
    default_begin()
  }
}
