import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(2)
    action.play_sound("Jump")
  }

  cancel() {}
}