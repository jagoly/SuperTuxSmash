import "actions/AirNeutral" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    fighter.play_sound("Swing1")
    action.enable_hitblobs("CLEAN")
    state.doLand = "LandAirNeutral"

    wait_until(5)
    action.disable_hitblobs(false)
    action.enable_hitblobs("LATE")

    wait_until(29)
    action.disable_hitblobs(true)

    wait_until(33)
    state.doLand = "MiscLand"

    wait_until(46) // 46
    default_end()
  }
}
