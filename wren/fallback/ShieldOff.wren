import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(1)
    //action.play_sound("ShieldOff")

    wait_until(8)
    action.allow_interrupt()
  }

  cancel() {}
}
