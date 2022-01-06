import "actions/LedgeClimb" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(14)
    fighter.play_sound("DashStart")

    wait_until(23)
    fighter.play_sound("StepRightHard")

    wait_until(30)
    vars.intangible = false

    wait_until(31)
    fighter.play_sound("StepLeftHard")

    wait_until(35) // 35
    default_end()
  }
}
