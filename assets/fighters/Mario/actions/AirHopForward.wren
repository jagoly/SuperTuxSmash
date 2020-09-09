import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    action.allow_interrupt()

    wait_until(1)
    action.play_sound("AirHop")
  }

  cancel() {
  }
}
