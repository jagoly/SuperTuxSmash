import "actions/Dodge" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    fighter.play_sound("Evade", false)
    vars.intangible = true

    wait_until(20)
    vars.intangible = false

    wait_until(25) // 32
    default_end()
  }
}
