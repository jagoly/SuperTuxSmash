import "actions/AirHopForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    // effect 30 no transform no cancel

    wait_until(1)
    fighter.play_sound("VoiceAirHop")

    wait_until(2)
    fighter.play_sound("AirHop")

    wait_until(12)
    fighter.play_sound("Whoosh2")
    wait_until(24)
    fighter.play_sound("Whoosh2")
    wait_until(36)
    fighter.play_sound("Whoosh3")
  }
}
