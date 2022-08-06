import "actions/NeutralComboA" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()
    _allowNext = false
    _autoJab = false

    wait_until(1)
    fighter.play_sound("Swing1", false)
    action.enable_hitblobs("")

    wait_until(3)
    action.disable_hitblobs(true)

    wait_until(6)
    _allowNext = true

    wait_until(9)
    _autoJab = true

    wait_until(15) // 16
    default_end()

    wait_until(23) // end combo
  }

  update() {
    if (_allowNext) {
      for (frame in ctrl.history) {
        if (frame.pressAttack) return "NeutralComboB"
      }
      if (ctrl.input.holdAttack) {
        if (vars.hitSomething) return "NeutralComboB"
        if (_autoJab) return "NeutralComboA"
      }
    }
  }
}
