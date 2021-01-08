import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(5)
    action.play_effect("HorizSmokeA")

    wait_until(9)
    action.play_sound("SwingExtra")
    action.play_sound("SwingFire")
    action.emit_particles("Fire")
    action.enable_hitblobs("")

    wait_until(12)
    action.disable_hitblobs()

    wait_until(43) // 52
    action.allow_interrupt()
  }

  cancel() {
    action.disable_hitblobs()
  }
}
