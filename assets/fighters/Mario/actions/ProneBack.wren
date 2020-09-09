import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    fighter.set_intangible(true)
    action.play_sound("Evade")

    wait_until(24)
    fighter.set_intangible(false)

    wait_until(34)
    action.allow_interrupt()
  }

  cancel() {
    fighter.set_intangible(false)
  }
}
