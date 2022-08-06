import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Neutral")
    fighter.play_animation("CrouchOff", 2, true)
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(2)
    //fighter.play_sound("CrouchOff", false)
  }
}
