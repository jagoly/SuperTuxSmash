import "actions/AirHopForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    action.play_sound("AirHop")
  }
}
