import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(5)
    action.set_flag_AllowNext()

    wait_until(6)
    action.play_sound("SmashStart")

    wait_until(65)
  }

  cancel() {
    action.cancel_sound("SmashStart")
  }
}