import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    fighter.set_velocity_x(0.0)

    wait_until(32)
    action.allow_interrupt()
  }

  cancel() {
    action.disable_hitblobs()
  }
}
