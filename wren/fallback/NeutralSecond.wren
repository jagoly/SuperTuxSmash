import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(8)
    action.set_flag_AllowNext()

    wait_until(16)
    action.set_flag_AutoJab()

    wait_until(24)
    action.allow_interrupt()

    wait_until(32)
  }

  cancel() {
    action.disable_hitblobs()
  }
}
