import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.play_animation("ThrowDown", 1, true)
    vars.victim.state.script.start_throw("ThrownDown/%(fighter.name)")
    state.disable_actions()
  }

  default_release() {
    action.throw_victim("THROW")
    vars.victim.state.script.finish_throw()
    vars.victim = null
    fighter.change_state("Action")
    state.doFall = "MiscFall"
  }

  default_end() {
    fighter.change_state("Neutral")
    fighter.set_next_animation("NeutralLoop", 0)
  }

  execute() {
    default_begin()

    wait_until(18)
    default_release()

    wait_until(39)
    default_end()
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
