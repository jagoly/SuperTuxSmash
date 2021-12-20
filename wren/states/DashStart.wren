import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // begin moving on the next frame
  enable_movement() {
    vars.velocity.x = attrs.dashSpeed * vars.facing
    vars.moveMobility = attrs.traction * 4.0
    vars.moveSpeed = attrs.dashSpeed
  }

  // disable reversing evade actions
  disable_reverse_evade() {
    _reverseEvade = false
  }

  // prevent dodges, fsmash, and pivot
  disable_early_actions() {
    _earlyActions = false
  }

  // allow changing to shield or dash
  enable_late_actions() {
    _lateActions = true
  }

  enter() {
    vars.edgeStop = "Input"
    vars.moveMobility = 0.0
    _reverseEvade = true
    _earlyActions = true
    _lateActions = false
    _inputHeld = ctrl.input.relIntX > 2 && ctrl.input.intY.abs < 3
  }

  update() {
    var r // return value

    // fall
    if (base_update_ground()) return "MiscFall"

    // dodges
    if (_earlyActions) {
      if (_reverseEvade) {
        if (r = lib.check_GroundDodgesRv(ctrl.input)) return r
      } else {
        if (r = lib.check_GroundDodges(ctrl.input)) return r
      }
    }

    // shield
    if (_lateActions) {
      if (ctrl.input.holdShield) return "ShieldOn"
    }

    // specials
    if (ctrl.input.pressSpecial) {
      if (_earlyActions) {
        if (ctrl.input.intX.abs > ctrl.input.intY.abs) {
          if (ctrl.input.relIntX >= 3) {
            vars.velocity.x = 0.0
            return "SpecialForward"
          }
          if (ctrl.input.relIntX <= -3) {
            vars.velocity.x = 0.0
            return "SpecialForwardRv"
          }
        }
      }
      if (ctrl.input.intY >= ctrl.input.intX.abs) {
        if (ctrl.input.intY >= 3) {
          return ctrl.input.relIntX >= 0 ? "SpecialUp" : "SpecialUpRv"
        }
      }
    }

    // smashes, dash attack
    if (ctrl.input.pressAttack) {
      if (_earlyActions) {
        if (ctrl.input.intX.abs >= ctrl.input.intY.abs) {
          if (ctrl.input.relModX == 1) {
            vars.velocity.x = 0.0
            return "ChargeForward"
          }
          if (ctrl.input.relModX == -1) {
            vars.velocity.x = 0.0
            return "ChargeForwardRv"
          }
        }
      }
      if (ctrl.input.intY > ctrl.input.intX.abs) {
        if (ctrl.input.modY == 1) return "ChargeUp"
      }
      else if (_inputHeld) {
        if (ctrl.input.relIntX >= 3) return "DashAttack"
      }
    }

    // jump
    if (ctrl.input.pressJump) return "JumpSquat"

    // pivot
    if (_earlyActions) {
      if (ctrl.input.intY.abs < 3) {
        if (ctrl.input.relMashX == -1) return "DashStartTurn"
      }
    }

    // check if input still held, change to dash
    if (_inputHeld) {
      if (ctrl.input.relIntX < 3 || ctrl.input.intY.abs > 2) _inputHeld = false
      else if (_lateActions) return "Dash"
    }
  }

  exit() {}
}
