import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(3)
    fighter.set_intangible(true)

    wait_until(4)
    action.play_sound("Evade")

    wait_until(12)
    action.play_sound("StepRightM")

    wait_until(19)
    fighter.set_intangible(false)

    wait_until(20)
    action.play_sound("StepLeftM")

    wait_until(32) // 34
    action.allow_interrupt()
  }

  cancel() {
    fighter.set_intangible(false)
  }
}
