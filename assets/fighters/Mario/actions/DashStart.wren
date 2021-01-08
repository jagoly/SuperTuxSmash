import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(4)
    action.play_effect("DashStart")

    wait_until(7)
    action.play_sound("Dash")

    wait_until(13)
    action.play_sound("StepRightM")

    wait_until(18)
    action.play_sound("StepLeftM")
  }

  cancel() {}
}