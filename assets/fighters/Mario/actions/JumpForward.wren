import "actions/JumpForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    action.play_sound("Jump")
  }
}
