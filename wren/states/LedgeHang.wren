import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // allow jumping, climbing, and dropping
  finish_catch() {
    _catchFinished = true
  }

  // prevent adjusting position upon state exit
  disable_exit_drop() {
    _exitDrop = false
  }

  enter() {
    vars.velocity.x = 0.0
    vars.velocity.y = 0.0
    vars.extraJumps = attrs.extraJumps
    vars.applyGravity = false
    vars.applyFriction = false
    vars.moveMobility = 0.0
    _catchFinished = false
    _exitDrop = true

    // steal the ledge from someone else
    if (vars.ledge.grabber) {
      vars.ledge.grabber.cancel_action()
    }
    vars.ledge.grabber = fighter

    // face towards the ledge
    if (vars.facing == vars.ledge.direction) {
      fighter.reverse_facing_slow(true, 2)
    }
  }

  update() {

    // action finished or ledge stolen
    if (!fighter.action) return "LedgeDrop"

    // must wait before being able to jump/climb/drop
    if (!_catchFinished) return

    // jump
    for (frame in ctrl.history) {
      if (frame.pressJump) return "LedgeJump"
    }

    // climb
    for (frame in ctrl.history) {
      if (frame.modY == 1) return "LedgeClimb"
      if (frame.relModX == 1) return "LedgeClimb"
    }

    // drop
    for (frame in ctrl.history) {
      if (frame.relModX == -1 || frame.modY == -1) return "LedgeDrop"
    }
  }

  exit() {

    // change position to match visual position
    if (_exitDrop) {
      vars.position.x =
        vars.ledge.position.x + vars.ledge.direction * diamond.halfWidth
      vars.position.y =
        vars.ledge.position.y - 0.75 * diamond.offsetTop
    }

    // clear the ledge, unless someone stole it
    if (vars.ledge.grabber == fighter) {
      vars.ledge.grabber = null
    }
    vars.ledge = null

    // prevent re-catching the ledge for a bit
    vars.noCatchTime = 48
  }
}
