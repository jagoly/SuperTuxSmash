import "actions/AirDodge" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(3)
    action.play_sound("Evade")
    vars.intangible = true

    wait_until(29)
    vars.intangible = false

    wait_until(50) // 50
    default_end()
  }
}
