import "actions/JumpSquat" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(5)
    state.second_last_frame()

    wait_until(6)
    return state.last_frame()
  }
}
