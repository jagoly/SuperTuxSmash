import "actions/GrabAttack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(15)
    action.enable_hitblobs("")

    wait_until(16)
    action.disable_hitblobs(true)

    wait_until(23) // 24
    default_end()
  }
}
