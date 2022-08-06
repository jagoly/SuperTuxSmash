import "actions/AirUp" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    state.doLand = "LandAirUp"

    wait_until(3)
    fighter.play_sound("Swing1", false)
    action.enable_hitblobs("")

    wait_until(9)
    action.disable_hitblobs(true)

    wait_until(15)
    state.doLand = "MiscLand"

    wait_until(29) // 34
    default_end()
  }
}
