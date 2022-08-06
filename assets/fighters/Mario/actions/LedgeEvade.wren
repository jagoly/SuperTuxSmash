import "actions/LedgeEvade" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(14)
    fighter.play_sound("DashStart", false)

    wait_until(20)
    // play effect 11

    wait_until(25)
    // play effect 28

    wait_until(28)
    fighter.play_sound("DashStart", false)

    wait_until(30)
    vars.intangible = false

    wait_until(35)
    // play effect 28

    wait_until(38)
    fighter.play_sound("StepLeftHard", false)

    wait_until(44)
    fighter.play_sound("StepRightHard", false)

    wait_until(50) // 50
    default_end()
  }
}
