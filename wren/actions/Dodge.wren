import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("Action")
    fighter.play_animation("Dodge", 1, true)
    state.doFall = "MiscFall"
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(1)
    vars.intangible = true

    wait_until(20)
    vars.intangible = false

    wait_until(28)
    default_end()
  }

  cancel() {
    vars.intangible = false
  }
}
