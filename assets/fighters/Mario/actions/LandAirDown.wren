import "actions/LandAirDown" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    action.enable_hitblobs("")

    wait_until(3)
    action.disable_hitblobs(true)

    wait_until(10)
    fighter.play_sound("LandHeavy")

    wait_until(19) // 31
    default_end()
  }

  cancel() {
    action.disable_hitblobs(true)
  }
}
