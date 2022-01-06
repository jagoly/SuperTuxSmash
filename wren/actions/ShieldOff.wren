import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Action")
    fighter.play_animation("ShieldOff", 2, true)
    state.doFall = "MiscFall"
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(1)
    //fighter.play_sound("ShieldOff")

    wait_until(12)
    default_end()
  }

  update() {
    // todo: maybe ShieldOff should be a state?
    if (ctrl.input.pressJump) return "JumpSquat"
  }
}
