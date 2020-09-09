import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(4)
    action.play_sound("LandTumble")

    wait_until(32)
    action.allow_interrupt()
  }

  cancel() {}
}
