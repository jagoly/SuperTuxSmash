import "actions/NeutralComboA" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    _allowNext = false

    wait_until(3)
    //fighter.play_sound("Swing1")
    action.enable_hitblobs("")

    wait_until(5)
    action.disable_hitblobs(true)

    wait_until(9)
    _allowNext = true

    wait_until(15) // 15
    default_end()

    wait_until(23) // end combo
  }

  update() {
    if (_allowNext) {
      for (frame in ctrl.history) {
        if (frame.pressAttack) return "NeutralComboB"
      }
      if (ctrl.input.holdAttack) return "NeutralComboB"
    }
  }
}
