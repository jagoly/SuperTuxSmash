import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Grabbed")
    if (vars.onGround) {
      fighter.play_animation("GrabbedStartLow", 4, true)
      fighter.set_next_animation("GrabbedLoopLow", 0)
    } else {
      fighter.play_animation("GrabbedStartHigh", 4, true)
      fighter.set_next_animation("GrabbedLoopHigh", 0)
    }
  }

  execute() {
    default_begin()
  }
}
