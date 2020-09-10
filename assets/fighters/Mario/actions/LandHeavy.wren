import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    action.emit_particles("Ring")

    wait_until(3)
    action.play_sound("LandHeavy")

    wait_until(8)
    action.allow_interrupt()
  }

  cancel() {}
}
