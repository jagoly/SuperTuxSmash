import "actions/SmashDown" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    fighter.play_sound("VoiceSmashDown", false)

    wait_until(2)
    action.play_effect("WhirlwindL") // todo particles
    fighter.play_sound("Swing3", false)
    fighter.play_sound("SwingExtra", false)
    action.enable_hitblobs("A")

    wait_until(4)
    action.disable_hitblobs(true)

    wait_until(11)
    fighter.play_sound("Swing1", false)
    fighter.play_sound("SwingExtra", false)
    action.enable_hitblobs("B")

    wait_until(12)
    action.disable_hitblobs(true)

    wait_until(36) // 36
    default_end()
  }
}
