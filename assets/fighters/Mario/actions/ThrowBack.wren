import "actions/ThrowBack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(1)
    fighter.play_sound("Whoosh5", false)
    fighter.play_sound("Whoosh7", false)

    wait_until(8)
    action.play_effect("ShockCrown")

    wait_until(15)
    action.play_effect("WhirlwindL") // todo particles

    wait_until(21)
    fighter.play_sound("Whoosh4", false)
    fighter.play_sound("Whoosh5", false)

    wait_until(28)
    action.play_effect("WhirlwindL") // todo particles

    wait_until(31)
    fighter.play_sound("Whoosh4", false)
    fighter.play_sound("Whoosh5", false)

    wait_until(41)
    fighter.play_sound("Whoosh4", false)
    fighter.play_sound("Whoosh5", false)
    fighter.play_sound("VoiceThrowBack", false)

    wait_until(43)
    action.play_effect("LandHeavy") // todo particles
    // PtcCommonSmashFlash at throw bone

    // note that brawl has this at frame 43
    // Brawl seems to have a frame of delay before the throw actually happens,
    // probably so that update order doesn't matter. STS ensures that victims
    // update after their bullies, so there is no need for a delay.
    wait_until(44)
    default_release()

    wait_until(67) // 68
    default_end()
  }
}
