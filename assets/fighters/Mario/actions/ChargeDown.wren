import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(2)
    action.set_flag_AllowNext()

    wait_until(3)
    action.play_sound("SmashStart")
    
    wait_until(62)
  }

  cancel() {
    action.cancel_sound("SmashStart")
  }
}