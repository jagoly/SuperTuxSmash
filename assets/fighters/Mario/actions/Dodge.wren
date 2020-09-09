import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(1)
    fighter.set_intangible(true)
    action.play_sound("Evade")

    wait_until(20)
    fighter.set_intangible(false)

    wait_until(25)
    action.allow_interrupt()
  }

  cancel() {
    fighter.set_intangible(false)
  }
}
