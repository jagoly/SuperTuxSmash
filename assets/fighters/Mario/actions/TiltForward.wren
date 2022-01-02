import "actions/TiltForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    allowAngle = false

    wait_until(4)
    action.play_sound("Swing2")
    if (angle == -1) {
      action.enable_hitblobs("D")
    } else {
      action.enable_hitblobs("F")
    }

    wait_until(7)
    action.disable_hitblobs(true)

    wait_until(24) // 33
    default_end()
  }
}
