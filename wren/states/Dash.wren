import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
    vars.edgeStop = "Input"
    vars.moveMobility = attrs.traction * 4.0
    vars.moveSpeed = attrs.dashSpeed
  }

  update() {
    var r // return value

    // advance animation based on velocity
    vars.animTime = vars.animTime + vars.velocity.x.abs / attrs.dashAnimSpeed

    // fall
    if (base_update_ground()) return "MiscFall"

    // dodges
    if (r = lib.check_GroundDodges(ctrl.input)) return r

    // grab
    if (ctrl.input.pressGrab) return "DashGrab"

    // shield
    if (ctrl.input.holdShield) return "ShieldOn"

    // specials
    if (r = lib.check_GroundSpecials(ctrl.input)) return r

    // smashes
    if (r = lib.check_GroundSmashes(ctrl.input)) return r

    // dash attack
    if (ctrl.input.pressAttack) return "DashAttack"

    // jump
    if (ctrl.input.pressJump) return "JumpSquat"

    // brake turn
    if (ctrl.input.relIntX <= -3) return "BrakeTurn"

    // brake
    if (ctrl.input.relIntX <= 2) return "Brake"
  }

  exit() {}
}
