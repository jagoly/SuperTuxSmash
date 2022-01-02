import "actions/AirHopForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    // effect 30 no transform no cancel

    wait_until(1)
    action.play_sound("VoiceAirHop")

    wait_until(2)
    action.play_sound("AirHop")

    wait_until(12)
    action.play_sound("Whoosh2")
    wait_until(24)
    action.play_sound("Whoosh2")
    wait_until(36)
    action.play_sound("Whoosh3")
  }
}
