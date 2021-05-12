import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(1)
    action.play_effect("LandHeavy")

    wait_until(3)
    action.play_sound("LandHeavy")

    wait_until(5)
    action.emit_particles("Ring")

    wait_until(8)
    action.allow_interrupt()
  }

  cancel() {}
}
