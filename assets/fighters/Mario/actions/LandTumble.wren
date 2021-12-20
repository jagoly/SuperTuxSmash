import "actions/LandTumble" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(4)
    action.play_sound("LandTumble")

    wait_until(32) // 40
    default_end()
  }
}
