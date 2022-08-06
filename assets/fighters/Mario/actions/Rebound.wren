import "actions/Rebound" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(vars.reboundTime)
    default_end()
  }
}
