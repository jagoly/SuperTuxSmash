import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    // todo:
    // Turns out we do need to logically reverse facing in the middle of an
    // animation. Something that would set facing right away, but visually keep
    // the old facing until the current animation ends.
    fighter.reverse_facing_auto()
    fighter.play_animation("ThrowBack", 1, true)
    vars.victim.state.script.start_throw("ThrownBack/%(fighter.name)")
    state.disable_actions()
  }

  default_release() {
    action.throw_victim("THROW")
    vars.victim.reverse_facing_auto()
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

    wait_until(44)
    default_release()

    wait_until(67)
    default_end()
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
