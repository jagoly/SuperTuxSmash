import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(1)
    //action.play_sound("ShieldOn")
  }

  cancel() {
    //action.cancel_sound("ShieldOn")
  }
}
