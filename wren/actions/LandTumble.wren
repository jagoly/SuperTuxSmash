import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    // todo: cancel_slow_rotation() method
    fighter.reverse_facing_auto()
    fighter.reverse_facing_auto()
    fighter.change_state("Action")
    fighter.play_animation("LandTumble", 2, true)
    state.doFall = "MiscTumble"
    vars.edgeStop = "Never"
    vars.flinch = true
    vars.extraJumps = attrs.extraJumps
    // todo: bouncing
  }

  default_end() {
    fighter.change_state("Prone")
    fighter.set_next_animation("ProneLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(32)
    default_end()
  }

  cancel() {
    vars.flinch = false
  }
}
