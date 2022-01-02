import "actions/Vertigo" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    action.play_sound("Vertigo")
  }
}
