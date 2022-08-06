import "actions/ShieldOff" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    fighter.play_sound("ShieldOff", false)

    wait_until(12) // 16
    default_end()
  }
}
