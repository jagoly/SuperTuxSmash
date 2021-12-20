import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // call to prevent changing to BrakeTurn
  disable_allow_turn() {
    _allowTurn = false
  }

  enter() {
    vars.edgeStop = "Input"
    vars.moveMobility = 0.0
    _allowTurn = true
  }

  update() {
    var r // return value

    // fall
    if (base_update_ground()) return "MiscFall"

    // dodges
    if (r = lib.check_GroundDodges(ctrl.input)) return r

    // shield
    if (ctrl.input.holdShield) return "ShieldOn"

    // specials
    if (r = lib.check_GroundSpecials(ctrl.input)) return r

    // smashes
    if (r = lib.check_GroundSmashes(ctrl.input)) return r

    // attacks
    if (r = lib.check_GroundAttacks(ctrl.input)) return r

    // jump
    if (ctrl.input.pressJump) return "JumpSquat"

    // turn
    if (_allowTurn) {
      if (ctrl.input.relIntX <= -3) return "BrakeTurn"
    }
  }

  exit() {}
}
