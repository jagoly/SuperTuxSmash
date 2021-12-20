import "actions/DashStart" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(4)
    action.play_effect("DashStart")

    wait_until(6)
    state.disable_early_actions()

    wait_until(7)
    action.play_sound("Dash")

    wait_until(10)
    state.enable_late_actions()

    wait_until(13)
    action.play_sound("StepRightM")

    wait_until(14)
    default_end()

    wait_until(18)
    action.play_sound("StepLeftM")
  }
}
