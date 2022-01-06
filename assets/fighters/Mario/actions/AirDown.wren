import "actions/AirDown" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(4)
    fighter.play_sound("VoiceAirDown")
    fighter.play_sound("SwingAirDown1")
    action.enable_hitblobs("ABCDE")
    state.doLand = "LandAirDown"

    wait_until(5)
    action.disable_hitblobs(true)

    wait_until(6)
    action.enable_hitblobs("ABCDE")

    wait_until(7)
    fighter.play_sound("SwingAirDown1")
    action.disable_hitblobs(true)

    wait_until(8)
    action.enable_hitblobs("ABCDE")

    wait_until(9)
    action.disable_hitblobs(true)

    wait_until(10)
    fighter.play_sound("SwingAirDown1")
    action.enable_hitblobs("ABCDE")

    wait_until(11)
    action.disable_hitblobs(true)

    wait_until(12)
    action.enable_hitblobs("ABCDE")

    wait_until(13)
    fighter.play_sound("SwingAirDown1")
    action.disable_hitblobs(true)

    wait_until(16)
    fighter.play_sound("SwingAirDown1")

    wait_until(19)
    fighter.play_sound("SwingAirDown1")

    wait_until(24)
    fighter.play_sound("SwingAirDown2")
    action.enable_hitblobs("Z")

    wait_until(26)
    action.disable_hitblobs(true)

    wait_until(32)
    state.doLand = "MiscLand"

    wait_until(37) // 45
    default_end()
  }
}
