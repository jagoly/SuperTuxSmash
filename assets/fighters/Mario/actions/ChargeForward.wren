import "actions/ChargeForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(5)
    allowNext = true

    wait_until(6)
    fighter.play_sound("SmashStart")
    fighter.play_sound("VoiceChargeForward")

    wait_until(66) // 66
    return "SmashForward"
  }

  cancel() {
    fighter.cancel_sound("SmashStart")
  }
}
