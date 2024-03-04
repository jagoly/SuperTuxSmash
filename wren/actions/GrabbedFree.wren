import "FighterAction" for FighterActionScript

// Velocity and height numbers are guesses, the wiki doesn't have any numbers.
// Could be tested with some effort, but it doesn't matter that much.

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    if (vars.onGround) {
      fighter.change_state("Action")
      fighter.play_animation("GrabbedFreeLow", 2, true)
      state.doFall = "MiscFall"
      // https://www.ssbwiki.com/Grab_release#Ground_release
      vars.velocity.x = vars.facing * -0.125
      vars.velocity.y = 0.0
    } else {
      fighter.change_state("ActionAir")
      fighter.play_animation("GrabbedFreeHigh", 2, true)
      state.doLand = "MiscLand"
      vars.moveMobility = 0.0
      // https://www.ssbwiki.com/Grab_release#Air_release
      vars.velocity.x = vars.facing * -0.125
      lib.assign_jump_velocity_y(2.5)
    }
  }

  default_end() {
    if (vars.onGround) {
      fighter.change_state("Neutral")
      fighter.set_next_animation("NeutralLoop", 0)
    } else {
      fighter.change_state("Fall")
      fighter.set_next_animation("FallLoop", 0)
    }
  }

  execute() {
    default_begin()

    wait_until(30)
    default_end()
  }
}
