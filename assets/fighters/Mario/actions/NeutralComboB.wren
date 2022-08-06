import "actions/GenericAttack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin("NeutralComboB")
    _allowNext = false
    _autoJab = false

    wait_until(1)
    fighter.play_sound("Swing2", false)
    action.enable_hitblobs("")

    wait_until(3)
    action.disable_hitblobs(true)

    wait_until(5)
    _allowNext = true

    wait_until(9)
    _autoJab = true

    wait_until(18) // 18
    default_end()

    wait_until(23) // end combo
  }

  update() {
    if (_allowNext) {
      for (frame in ctrl.history) {
        if (frame.pressAttack) return "NeutralComboC"
      }
      if (ctrl.input.holdAttack) {
        if (vars.hitSomething) return "NeutralComboC"
        if (_autoJab) return "NeutralComboA"
      }
    }
  }
}
