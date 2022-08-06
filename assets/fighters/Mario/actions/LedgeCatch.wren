import "actions/LedgeCatch" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    //wait_until(1)
    // play effect

    wait_until(2)
    fighter.play_sound("LedgeCatch", false)

    wait_until(4)
    fighter.play_sound("VoiceLedgeCatch", false)

    wait_until(21) // 21
    default_end()
  }
}
