import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // allow starting new actions
  finish_catch() {
    _catchFinished = true
  }

  enter() {
    vars.velocity.x = 0.0
    vars.velocity.y = 0.0
    vars.extraJumps = attrs.extraJumps
    _catchFinished = false

    // steal the ledge from someone else
    if (vars.ledge.grabber) {
      vars.ledge.grabber.cancel_action()
    }
    vars.ledge.grabber = fighter

    // face towards the ledge
    if (vars.facing == vars.ledge.direction) {
      fighter.reverse_facing_slow(true, 2)
    }

    // set attachment point
    vars.attachPoint.x = vars.ledge.position.x
    vars.attachPoint.y = vars.ledge.position.y
  }

  update() {

    // action finished or ledge stolen
    if (!fighter.action) return "LedgeDrop"

    // must wait before being able to start new actions
    if (!_catchFinished) return

    // evade
    for (frame in ctrl.history) {
      if (frame.pressShield) return "LedgeEvade"
    }

    // attack
    for (frame in ctrl.history) {
      if (frame.pressAttack) return "LedgeAttack"
    }

    // jump
    for (frame in ctrl.history) {
      if (frame.pressJump) return "LedgeJump"
    }

    // climb
    for (frame in ctrl.history) {
      if (frame.relModX == 1 || frame.modY == 1) return "LedgeClimb"
    }

    // drop
    for (frame in ctrl.history) {
      if (frame.relModX == -1 || frame.modY == -1) return "LedgeDrop"
    }
  }

  exit() {

    // clear the ledge, unless someone stole it
    if (vars.ledge.grabber == fighter) {
      vars.ledge.grabber = null
    }
    vars.ledge = null

    // prevent re-catching the ledge for a bit
    vars.noCatchTime = 48
  }
}
