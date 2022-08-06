import "actions/AirBack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(5)
    lib.play_random_voice_attack()
    fighter.play_sound("Swing1", false)
    action.enable_hitblobs("CLEAN")
    state.doLand = "LandAirBack"

    wait_until(8)
    action.disable_hitblobs(false)
    action.enable_hitblobs("LATE")

    wait_until(13)
    action.disable_hitblobs(true)

    wait_until(18)
    state.doLand = "MiscLand"

    wait_until(33) // 46
    default_end()
  }
}
