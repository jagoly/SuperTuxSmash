import "actions/ProneAttack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(18)
    fighter.play_sound("Swing3", false)
    action.enable_hitblobs("A")

    wait_until(20)
    action.disable_hitblobs(true)

    wait_until(24)
    fighter.play_sound("Swing3", false)
    action.enable_hitblobs("B")

    wait_until(26)
    action.disable_hitblobs(true)
    vars.intangible = false

    wait_until(50) // 50
    default_end()
  }
}
