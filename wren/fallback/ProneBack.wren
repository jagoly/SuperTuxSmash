import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    fighter.set_intangible(true)

    wait_until(24)
    fighter.set_intangible(false)

    wait_until(32)
    action.allow_interrupt()
  }

  cancel() {
    fighter.set_intangible(false)
  }
}
