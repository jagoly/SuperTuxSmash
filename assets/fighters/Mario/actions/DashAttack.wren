import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    fighter.set_velocity_x(0.0)

    wait_until(5)
    action.enable_hitblobs("CLEAN")

    wait_until(6)
    action.play_sound("SwingMedium")

    wait_until(9)
    action.disable_hitblobs()
    action.enable_hitblobs("LATE")

    wait_until(25)
    action.disable_hitblobs()

    wait_until(37) // 54
    action.allow_interrupt()
  }

  cancel() {
    action.disable_hitblobs()
  }
}
