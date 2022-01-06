import "actions/EvadeForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(3)
    vars.intangible = true

    wait_until(4)
    fighter.play_sound("Evade")

    wait_until(12)
    fighter.play_sound("StepRightHard")

    wait_until(19)
    vars.intangible = false

    wait_until(20)
    fighter.play_sound("StepLeftHard")

    wait_until(32) // 36
    default_end()
  }
}
