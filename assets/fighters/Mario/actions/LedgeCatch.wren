import "actions/LedgeCatch" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    //wait_until(1)
    // play effect

    wait_until(2)
    action.play_sound("LedgeCatch")

    wait_until(4)
    action.play_sound("VoiceLedgeCatch")

    wait_until(21) // 21
    default_end()
  }
}
