import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Shield")
    fighter.play_animation("ShieldOn", 2, true)
    fighter.set_next_animation("ShieldLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(1)
    //action.play_sound("ShieldOn")
  }
}
