import "actions/LandAirForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    action.emit_particles("Ring")

    wait_until(10)
    fighter.play_sound("LandHeavy")

    wait_until(26) // 31
    default_end()
  }
}
