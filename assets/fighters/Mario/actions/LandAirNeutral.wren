import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(2)
    action.emit_particles("Ring")

    wait_until(10)
    action.play_sound("LandHeavy")
    action.allow_interrupt()
  }

  cancel() {}
}
