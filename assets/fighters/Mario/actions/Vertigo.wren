import "actions/Vertigo" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    fighter.play_sound("VoiceVertigo")
  }
}
