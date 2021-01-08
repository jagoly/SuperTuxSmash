import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(6)
    action.set_flag_AllowNext()

    wait_until(7)
    action.play_sound("SmashStart")

    wait_until(66)
  }

  cancel() {
    action.cancel_sound("SmashStart")
  }
}