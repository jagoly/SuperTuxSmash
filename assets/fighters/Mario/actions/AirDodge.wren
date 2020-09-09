import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(3)
    fighter.set_intangible(true)
    action.play_sound("Evade")

    wait_until(29)
    fighter.set_intangible(false)

    wait_until(49) // 50
    action.allow_interrupt()
  }
  
  cancel() {
    fighter.set_intangible(false)
  }
}
