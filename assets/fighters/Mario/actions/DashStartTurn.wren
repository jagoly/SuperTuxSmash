import "actions/DashStartTurn" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    state.enable_movement()

    wait_until(3)
    state.disable_reverse_evade()

    wait_until(6)
    action.play_effect("DashStart")
    state.disable_early_actions()

    wait_until(8)
    fighter.play_sound("DashStart")

    wait_until(10)
    state.enable_late_actions()

    wait_until(13)
    fighter.play_sound("StepRightHard")

    wait_until(14)
    default_end()

    wait_until(18)
    fighter.play_sound("StepLeftHard")
  }
}
