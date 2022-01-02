import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  // must be called one frame before we change state
  set_vertical_velocity() {
    lib.assign_jump_velocity_y(attrs.jumpHeight)
  }

  // must be called on the frame we change state
  set_horizontal_velocity() {
    // todo: calculate velocity to move a specific distance
    vars.velocity.x = attrs.airSpeed * vars.facing
  }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("Action")
    fighter.play_animation("LedgeJumpA", 1, true)
    state.doFall = null
    vars.intangible = true
  }

  default_end() {
    fighter.change_state("Fall")
    fighter.play_animation("LedgeJumpB", 0, true)
    fighter.set_next_animation("FallLoop", 0)
    vars.intangible = false
  }

  execute() {
    default_begin()

    wait_until(15)
    set_vertical_velocity()

    wait_until(16)
    set_horizontal_velocity()
    default_end()
  }

  cancel() {
    vars.intangible = false
  }
}
