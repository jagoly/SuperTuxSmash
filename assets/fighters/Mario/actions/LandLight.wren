import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(1)
    action.emit_particles("Ring")
    action.play_effect("LandLight")
    action.play_sound("LandLight")

    wait_until(2)
    action.allow_interrupt()
  }

  cancel() {}
}
