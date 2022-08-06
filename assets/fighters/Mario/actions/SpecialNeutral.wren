import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  execute() {
    ctrl.clear_history()
    fighter.change_state("Special")
    state.doLand = Fn.new {
      fighter.play_animation("SpecialNeutral", 0, false)
      vars.moveMobility = 0.0
    }
    state.doFall = Fn.new {
      fighter.play_animation("SpecialAirNeutral", 0, false)
      vars.moveMobility = attrs.airMobility
      vars.moveSpeed = attrs.airSpeed
    }
    if (vars.onGround) {
      fighter.play_animation("SpecialNeutral", 1, true)
      vars.moveMobility = 0.0
      // play visual effect
    } else {
      fighter.play_animation("SpecialAirNeutral", 1, true)
      vars.moveMobility = attrs.airMobility
      vars.moveSpeed = attrs.airSpeed
    }
    state.canReverse = true

    wait_until(5)
    // visual effect
    state.canReverse = false

    wait_until(11)
    fighter.play_sound("FireBallLaunch", false)

    wait_until(12)
    // visual effect
    fighter.play_sound("FireBallBloop", false)

    wait_until(13)
    fighter.spawn_article("FireBall")

    wait_until(14)
    // flash effect overlay?

    wait_until(16)
    // remove flash effect
    // flash effect light?

    wait_until(21)
    // set flash effect light colour

    wait_until(26)
    // remove flash effect

    wait_until(43) // 49
    if (vars.onGround) {
      fighter.change_state("Neutral")
      fighter.set_next_animation("NeutralLoop", 0)
    } else {
      fighter.change_state("Fall")
      fighter.set_next_animation("FallLoop", 0)
    }
  }

  cancel() {
    //action.disable_hitblobs(true)
  }
}
