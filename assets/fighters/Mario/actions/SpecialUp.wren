import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  execute() {
    ctrl.clear_history()
    fighter.change_state("Special")
    state.doFall = null
    state.doLand = null
    vars.applyGravity = false
    vars.moveMobility = 0.0
    if (vars.onGround) {
      fighter.play_animation("SpecialUp", 1, true)
    } else {
      fighter.play_animation("SpecialAirUp", 1, true)
    }
    state.canReverse = true

    wait_until(2)
    state.canReverse = false
    action.enable_hitblobs("A")
    action.play_sound("SuperJump")
    // todo: proper angling
    vars.intangible = true
    // visual effect

    wait_until(3)
    vars.velocity.x = 0.0
    vars.velocity.y = 0.0
    //vars.moveMobility = attrs.airMobility
    //vars.moveSpeed = attrs.airSpeed
    // visual effect

    wait_until(5)
    state.doLand = "LandHelpless"

    wait_until(6)
    action.disable_hitblobs(true)
    action.enable_hitblobs("BCD")
    vars.intangible = false

    wait_until(7)
    action.disable_hitblobs(true)
    action.enable_hitblobs("BCD")

    wait_until(8)
    action.disable_hitblobs(true)
    action.enable_hitblobs("BCD")

    wait_until(9)
    action.disable_hitblobs(true)
    action.enable_hitblobs("EF")
    // only allow ledge grab in front

    wait_until(11)
    action.disable_hitblobs(true)
    action.enable_hitblobs("EF")

    wait_until(13)
    action.disable_hitblobs(true)
    action.enable_hitblobs("Z")

    wait_until(15)
    action.disable_hitblobs(true)
    // allow all ledge grabbing

    wait_until(38) // 38
    fighter.change_state("Helpless")
    fighter.set_next_animation("HelplessLoop", 0)
  }

  cancel() {
    action.disable_hitblobs(true)
    vars.intangible = false
  }
}
