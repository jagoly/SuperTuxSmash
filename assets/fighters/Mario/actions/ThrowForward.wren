import "actions/ThrowForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    fighter.play_sound("Whoosh5", false)
    fighter.play_sound("Whoosh7", false)

    wait_until(12)
    action.play_effect("AttackSmoke") // todo particles
    // PtcCommonSmashFlash at throw bone
    lib.play_random_voice_attack()
    fighter.play_sound("Whoosh4", false)
    fighter.play_sound("Whoosh5", false)

    // note that brawl has this at frame 12
    // see ThrowBack for details
    wait_until(13)
    default_release()

    wait_until(27) // 28
    default_end()
  }
}
