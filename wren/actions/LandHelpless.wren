import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    fighter.change_state("Action")
    fighter.play_animation("LandHelpless", 1, true)
    state.doFall = "MiscFall"
    vars.extraJumps = attrs.extraJumps
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(24)
    default_end()
  }
}