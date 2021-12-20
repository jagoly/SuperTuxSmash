import "FighterAction" for FighterActionScript

// in brawl, ground/air versions of this are slightly different
// from sm4sh onwards they are the same, so we do that too

class Script is FighterActionScript {
  construct new(a) { super(a) }

  execute() {
    ctrl.clear_history()
    fighter.change_state("Special")
    state.doLand = Fn.new {
      fighter.play_animation("SpecialForward", 0, false)
    }
    state.doFall = Fn.new {
      fighter.play_animation("SpecialAirForward", 0, false)
    }
    state.canReverse = true
    vars.velocity.x = vars.velocity.x * 0.5
    vars.moveMobility = 0.0
    if (vars.onGround) {
      fighter.play_animation("SpecialForward", 1, true)
      // play visual effect
    } else {
      fighter.play_animation("SpecialAirForward", 1, true)
    }
    // disable item visibility
    // enable cape article

    wait_until(5)
    state.canReverse = false
    // enable projectile reflect

    wait_until(7)
    // action.play_sound("")

    wait_until(10)
    vars.applyGravity = false
    // visual effect

    wait_until(11)
    //action.enable_hitblobs("")
    // visual effect
    // graphic effect?
    // action.play_sound("")
    // action.play_sound("")

    wait_until(12)
    // wind effect

    wait_until(14)
    action.disable_hitblobs(true)

    wait_until(22)
    // end wind effect

    // in brawl, ground=30, air=33
    wait_until(32)
    // disable projectile reflect

    wait_until(33)
    vars.applyGravity = true

    wait_until(36) // 36
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
    vars.applyGravity = true
  }
}
