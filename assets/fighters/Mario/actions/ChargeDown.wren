import "actions/ChargeDown" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    allowNext = true

    wait_until(3)
    fighter.play_sound("SmashStart", true)

    wait_until(63) // 63
    return "SmashDown"
  }
}
