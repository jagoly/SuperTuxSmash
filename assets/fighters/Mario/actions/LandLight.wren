import "actions/LandLight" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    action.emit_particles("Ring")
    action.play_effect("LandLight")
    action.play_sound("LandLight")

    wait_until(2) // 16
    default_end()
  }
}
