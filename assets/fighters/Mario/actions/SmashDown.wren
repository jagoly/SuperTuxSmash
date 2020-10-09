import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(2)
    action.play_sound("SwingExtra")
    action.play_sound("SwingLarge")
    action.enable_hitblobs("FRONT")

    wait_until(4)
    action.disable_hitblobs()
    fighter.reset_collisions()

    wait_until(11)
    action.play_sound("SwingExtra")
    action.play_sound("SwingSmall")
    action.enable_hitblobs("BACK")

    wait_until(12)
    action.disable_hitblobs()

    wait_until(35) // 36
    action.allow_interrupt()
  }

  cancel() {
    action.disable_hitblobs()
  }
}
