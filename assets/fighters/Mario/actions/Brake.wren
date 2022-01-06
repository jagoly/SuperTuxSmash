import "actions/Brake" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    action.emit_particles("BrakeSmoke")

    wait_until(4)
    action.emit_particles("BrakeSmoke")

    wait_until(10)
    fighter.play_sound("Brake")
    state.disable_allow_turn()

    wait_until(13) // 23
    default_end()
  }
}
