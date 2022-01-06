import "actions/BrakeTurn" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(5)
    action.emit_particles("BrakeSmoke")

    wait_until(6)
    state.disable_reverse_evade()

    wait_until(10)
    fighter.play_sound("Brake")
    action.emit_particles("BrakeSmoke")

    wait_until(14)
    state.enable_movement()
    state.enable_stop()

    wait_until(21) // 21
    default_end()
  }
}
