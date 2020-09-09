import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    fighter.set_intangible(true)

    wait_until(2)
    action.play_sound("Stand")

    wait_until(22)
    fighter.set_intangible(false)

    wait_until(28)
    action.allow_interrupt()
  }

  cancel() {
    fighter.set_intangible(false)
  }
}
