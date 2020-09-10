import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    action.allow_interrupt()

    wait_until(7)
    action.play_sound("Dash")

    wait_until(13)
    action.play_sound("StepRightM")

    wait_until(18)
    action.play_sound("StepLeftM")
  }

  cancel() {
  }
}