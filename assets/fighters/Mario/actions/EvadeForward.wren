import "actions/EvadeForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(3)
    vars.intangible = true

    wait_until(4)
    action.play_sound("Evade")

    wait_until(12)
    action.play_sound("StepRightM")

    wait_until(19)
    vars.intangible = false

    wait_until(20)
    action.play_sound("StepLeftM")

    wait_until(32) // 36
    default_end()
  }
}
