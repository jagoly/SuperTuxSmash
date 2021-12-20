import "actions/LedgeJump" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(14)
    set_vertical_velocity()

    wait_until(15) // 15
    set_horizontal_velocity()
    default_end()
  }
}
