import "actions/LedgeCatch" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    position_frame_0()

    wait_until(1)
    position_frame_1()

    wait_until(2)
    position_frame_2()

    wait_until(21) // 21
    default_end()
  }
}
