import "actions/SmashUp" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    fighter.play_sound("VoiceSmashUp", false)

    wait_until(2)
    fighter.disable_hurtblob("HeadN")
    fighter.play_sound("Swing3", false)
    fighter.play_sound("SwingExtra", false)
    action.enable_hitblobs("")

    wait_until(8)
    fighter.enable_hurtblob("HeadN")
    action.disable_hitblobs(true)

    wait_until(34) // 34
    default_end()
  }

  cancel() {
    fighter.enable_hurtblob("HeadN")
    action.disable_hitblobs(true)
  }
}
