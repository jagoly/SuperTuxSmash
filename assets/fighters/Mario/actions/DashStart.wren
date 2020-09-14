import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    // current effect looks pretty bad,
    // could likely make it look better
    // just with more emitters, but the 
    // visual effects system needs work
    // anyway, so not bothering

    wait_until(3)
    action.emit_particles("Zoom")

    wait_until(5)
    action.emit_particles("Zoom")

    wait_until(7)
    action.emit_particles("Zoom")
    action.play_sound("Dash")

    wait_until(9)
    action.emit_particles("Zoom")

    wait_until(13)
    action.play_sound("StepRightM")

    wait_until(18)
    action.play_sound("StepLeftM")
  }

  cancel() {}
}