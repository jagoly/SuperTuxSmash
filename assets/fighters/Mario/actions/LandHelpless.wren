import "actions/LandHelpless" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    action.emit_particles("Ring")
    fighter.play_sound("LandHeavy")

    wait_until(31) // 31
    default_end()
  }
}
