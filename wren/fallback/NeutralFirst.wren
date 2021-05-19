import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(6)
    action.set_flag_AllowNext()

    wait_until(12)
    action.set_flag_AutoJab()

    wait_until(18)
    action.allow_interrupt()

    wait_until(24)
  }

  cancel() {
    action.disable_hitblobs()
  }
}
