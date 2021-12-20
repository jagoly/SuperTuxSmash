import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  // set true to allow changing to Attack
  allowNext { _allowNext }
  allowNext=(value) { _allowNext = value }

  // change to Attack if allowNext is also true
  attackReleased { _attackReleased }
  attackReleased=(value) { _attackReleased = value }

  default_begin() {
    ctrl.clear_history()
    fighter.change_state("Action")
    fighter.play_animation("ChargeDown", 1, true)
    state.doFall = "MiscFall"
    allowNext = false
    attackReleased = false
  }

  execute() {
    default_begin()

    wait_until(5)
    allowNext = true

    wait_until(6)
    //action.play_sound("SmashStart")

    wait_until(64)
    return "SmashDown"
  }

  update() {
    if (!ctrl.input.holdAttack) {
      attackReleased = true
    }
    if (allowNext && attackReleased) {
      return "SmashDown"
    }
  }

  cancel() {
    //action.cancel_sound("SmashStart")
  }
}