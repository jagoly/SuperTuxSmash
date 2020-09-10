import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    action.emit_particles("Ring")

    wait_until(1)
    action.play_sound("LandLight")

    wait_until(2)
    action.allow_interrupt()
  }

  cancel() {}
}
