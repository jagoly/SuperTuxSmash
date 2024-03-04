import "actions/DashGrab" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(5)
    fighter.play_sound("Whoosh3", false)

    wait_until(9)
    // stop sound

    wait_until(11)
    action.enable_hitblobs("")

    wait_until(13)
    action.disable_hitblobs(true)

    wait_until(15)
    fighter.play_sound("StepRightHard", false)

    wait_until(25)
    fighter.play_sound("StepLeftHard", false)

    wait_until(37) // 54
    default_end()
  }
}
