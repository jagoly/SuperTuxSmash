import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(4)
    fighter.set_autocancel(false)
    action.play_sound("Spin")
    action.enable_hitblobs("A")

    wait_until(5)
    action.disable_hitblobs()
    fighter.reset_collisions()

    wait_until(6)
    action.enable_hitblobs("B")

    wait_until(7)
    action.play_sound("Spin")
    action.disable_hitblobs()
    fighter.reset_collisions()

    wait_until(8)
    action.enable_hitblobs("C")

    wait_until(9)
    action.disable_hitblobs()
    fighter.reset_collisions()

    wait_until(10)
    action.play_sound("Spin")
    action.enable_hitblobs("D")

    wait_until(11)
    action.disable_hitblobs()
    fighter.reset_collisions()

    wait_until(12)
    action.enable_hitblobs("E")

    wait_until(13)
    action.play_sound("Spin")
    action.disable_hitblobs()
    fighter.reset_collisions()

    wait_until(16)
    action.play_sound("Spin")

    wait_until(19)
    action.play_sound("Spin")

    wait_until(24)
    action.play_sound("SpinLoud")
    action.enable_hitblobs("FINAL")

    wait_until(26)
    action.disable_hitblobs()

    wait_until(32)
    fighter.set_autocancel(true)

    wait_until(37) // 45
    action.allow_interrupt()
  }

  cancel() {
    fighter.set_autocancel(true)
    action.disable_hitblobs()
  }
}
