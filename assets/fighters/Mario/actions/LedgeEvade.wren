import "actions/LedgeEvade" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(14)
    action.play_sound("DashStart")

    wait_until(20)
    // play effect 11

    wait_until(25)
    // play effect 28

    wait_until(28)
    action.play_sound("DashStart")

    wait_until(30)
    vars.intangible = false

    wait_until(35)
    // play effect 28

    wait_until(38)
    action.play_sound("StepLeftHard")

    wait_until(44)
    action.play_sound("StepRightHard")

    wait_until(50) // 50
    default_end()
  }
}
