import "actions/CrouchOn" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2) // 8
    fighter.play_sound("CrouchOn", false)
  }
}
