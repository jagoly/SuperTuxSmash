import "actions/GrabFree" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    fighter.play_sound("LandLight", false)

    wait_until(30)
    default_end()
  }
}
