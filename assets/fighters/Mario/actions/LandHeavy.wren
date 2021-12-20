import "actions/LandHeavy" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    action.play_effect("LandHeavy")

    wait_until(3)
    action.play_sound("LandHeavy")

    wait_until(5)
    action.emit_particles("Ring")

    wait_until(8) // 31
    default_end()
  }
}
