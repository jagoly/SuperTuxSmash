import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(16)
    action.allow_interrupt()
  }

  cancel() {}
}
