import "actions/Turn" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(6)
    state.disable_reverse_evade()

    wait_until(10)
    default_end()
  }
}
