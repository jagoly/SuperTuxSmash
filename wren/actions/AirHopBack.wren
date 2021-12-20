import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("Fall")
    fighter.play_animation("AirHopBack", 2, true)
    fighter.set_next_animation("FallLoop", 1)
    lib.assign_jump_velocity_x()
    lib.assign_jump_velocity_y(attrs.airHopHeight)
    vars.extraJumps = vars.extraJumps - 1
  }

  execute() {
    default_begin()
  }
}
