import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    action.emit_particles("BrakeSmoke")
    action.allow_interrupt()

    wait_until(4)
    action.emit_particles("BrakeSmoke")

    wait_until(10)
    action.play_sound("Brake")
  }

  cancel() {
  }
}