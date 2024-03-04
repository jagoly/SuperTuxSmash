import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Grab")
    fighter.play_animation("GrabLoop", 1, true)
  }

  execute() {
    default_begin()
  }
}
