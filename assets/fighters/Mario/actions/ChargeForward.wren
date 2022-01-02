import "actions/ChargeForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(5)
    allowNext = true

    wait_until(6)
    action.play_sound("SmashStart")
    action.play_sound("VoiceChargeFwd")

    wait_until(66) // 66
    return "SmashForward"
  }

  cancel() {
    action.cancel_sound("SmashStart")
  }
}
