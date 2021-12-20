import "actions/TiltDown" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(4)
    action.play_sound("SwingMedium")
    action.enable_hitblobs("")

    wait_until(7)
    action.disable_hitblobs(true)

    wait_until(34) // 35
    default_end()
  }
}
