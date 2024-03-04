import "actions/NeutralGrab" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(5)
    action.enable_hitblobs("")
    fighter.play_sound("Whoosh3", false)

    wait_until(7)
    action.disable_hitblobs(true)

    wait_until(9)
    // stop sound

    wait_until(30) // 30
    default_end()
  }
}
