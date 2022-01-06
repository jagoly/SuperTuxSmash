import "actions/ChargeUp" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(6)
    allowNext = true

    wait_until(7)
    fighter.play_sound("SmashStart")

    wait_until(67) // 67
    return "SmashUp"
  }

  cancel() {
    fighter.cancel_sound("SmashStart")
  }
}
