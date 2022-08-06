import "actions/ProneStand" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    fighter.play_sound("CrouchOff", false)

    wait_until(22)
    vars.intangible = false

    wait_until(30) // 30
    default_end()
  }
}
