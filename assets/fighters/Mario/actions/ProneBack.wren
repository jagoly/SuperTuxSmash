import "actions/ProneBack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    fighter.play_sound("Evade", false)

    wait_until(23)
    vars.intangible = false

    wait_until(36) // 36
    default_end()
  }
}
