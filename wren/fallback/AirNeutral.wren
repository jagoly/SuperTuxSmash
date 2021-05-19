import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(2)
    fighter.set_autocancel(false)

    wait_until(20)
    fighter.set_autocancel(true)

    wait_until(32)
    action.allow_interrupt()
  }
  
  cancel() {
    fighter.set_autocancel(true)
    action.disable_hitblobs()
  }
}
