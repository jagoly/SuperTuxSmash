import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Vertigo")
    fighter.play_animation("VertigoStart", 2, true)
    fighter.set_next_animation("VertigoLoop", 0)
  }

  execute() {
    default_begin()
  }
}
