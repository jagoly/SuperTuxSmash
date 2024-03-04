import "actions/ThrowUp" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    fighter.play_sound("Whoosh5", false)
    fighter.play_sound("Whoosh7", false)

    wait_until(16)
    lib.play_random_voice_attack()
    fighter.play_sound("Whoosh4", false)
    fighter.play_sound("Whoosh5", false)

    // note that brawl has this at frame 17
    // see ThrowBack for details
    wait_until(18)
    default_release()
    // PtcCommonSmashFlashS

    wait_until(39) // 40
    default_end()
  }
}
