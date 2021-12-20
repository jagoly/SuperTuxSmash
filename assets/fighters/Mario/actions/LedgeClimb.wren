import "actions/LedgeClimb" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(30)
    vars.intangible = false

    wait_until(35) // 35
    default_end()
  }
}
