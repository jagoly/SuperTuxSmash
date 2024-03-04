import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    // todo: facing change should be mid animation
    fighter.reverse_facing_auto()
    fighter.change_state("Action")
    fighter.play_animation("EvadeForward", 2, true)
    state.doFall = "MiscFall"
    vars.velocity.x = 0.0
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(3)
    vars.intangible = true

    wait_until(19)
    vars.intangible = false

    wait_until(32)
    default_end()
  }

  cancel() {
    vars.intangible = false
  }
}
