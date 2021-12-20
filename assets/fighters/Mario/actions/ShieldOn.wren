import "actions/ShieldOn" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1) // 8
    action.play_sound("ShieldOn")
  }
}
