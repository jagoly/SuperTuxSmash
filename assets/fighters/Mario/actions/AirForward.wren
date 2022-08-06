import "actions/AirForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    state.doLand = "LandAirForward"

    wait_until(15)
    fighter.play_sound("VoiceAirForward", false)
    fighter.play_sound("Swing1", false)
    action.enable_hitblobs("EARLY")

    wait_until(16)
    action.disable_hitblobs(false)
    action.enable_hitblobs("CLEAN")

    wait_until(19)
    action.disable_hitblobs(false)
    action.enable_hitblobs("LATE")

    wait_until(21)
    action.disable_hitblobs(true)

    wait_until(42)
    state.doLand = "MiscLand"

    wait_until(59) // 75
    default_end()
  }
}
