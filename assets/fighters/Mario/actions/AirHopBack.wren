import "actions/AirHopBack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    // effect 30 no transform no cancel

    wait_until(1)
    fighter.play_sound("VoiceAirHop")

    wait_until(2)
    fighter.play_sound("AirHop")

    wait_until(10)
    fighter.play_sound("Whoosh2")
    wait_until(22)
    fighter.play_sound("Whoosh2")
    wait_until(34)
    fighter.play_sound("Whoosh2")
    wait_until(46)
    fighter.play_sound("Whoosh3")
    wait_until(58)
    fighter.play_sound("Whoosh3")
    wait_until(70)
    fighter.play_sound("Whoosh3")
  }
}
