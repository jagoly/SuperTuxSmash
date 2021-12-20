import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  get_anim_name() {
    if (vars.onGround) {
      if (_animGroup == 0) return "SpecialDownStart"
      if (_animGroup == 1) return "SpecialDownLoop"
      if (_animGroup == 2) return "SpecialDownLight"
      if (_animGroup == 3) return "SpecialDownHeavy"
    } else {
      if (_animGroup == 0) return "SpecialAirDownStart"
      if (_animGroup == 1) return "SpecialAirDownLoop"
      if (_animGroup == 2) return "SpecialAirDownLight"
      if (_animGroup == 3) return "SpecialAirDownHeavy"
    }
  }

  execute() {
    ctrl.clear_history()
    fighter.change_state("Special")
    state.doLand = Fn.new {
      fighter.play_animation(get_anim_name(), 0, false)
    }
    state.doFall = Fn.new {
      fighter.play_animation(get_anim_name(), 0, false)
    }
    state.canReverse = false
    vars.moveMobility = 0.0
    _animGroup = 0
    fighter.play_animation(get_anim_name(), 1, true)

    wait_until(19)
    _animGroup = 1
    fighter.play_animation(get_anim_name(), 1, true)

    wait_until(80)
    _animGroup = 2
    fighter.play_animation(get_anim_name(), 1, true)

    wait_until(128)
    if (vars.onGround) {
      fighter.change_state("Neutral")
      fighter.set_next_animation("NeutralLoop", 0)
    } else {
      fighter.change_state("Fall")
      fighter.set_next_animation("FallLoop", 0)
    }
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
