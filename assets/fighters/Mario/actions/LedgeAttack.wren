import "actions/LedgeAttack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(14)
    fighter.play_sound("DashStart")

    wait_until(16)
    // graphic id 16, RToeN, trans 0,0,-3 anchor

    wait_until(20)
    vars.intangible = false

    wait_until(23)
    action.enable_hitblobs("")
    fighter.play_sound("Swing3")

    wait_until(24)
    // graphic id 33

    wait_until(26)
    action.disable_hitblobs(true)

    wait_until(37)
    // graphic id 26

    wait_until(56) // 56
    default_end()
  }
}
