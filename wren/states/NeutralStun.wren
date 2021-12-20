import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
    vars.edgeStop = "Never"
    vars.applyGravity = true
    vars.applyFriction = true
    vars.flinch = true
    vars.moveMobility = 0.0
  }

  update() {
    var done = base_update_stun()
    var fall = base_update_ground()

    if (done) {
      if (fall) {
        fighter.change_state("Tumble")
        fighter.play_animation("TumbleLoop", 4, true)
      } else {
        fighter.change_state("Neutral")
      }
    }
    else if (fall) {
      fighter.change_state("TumbleStun")
      fighter.play_animation("TumbleLoop", 4, true)
    }
  }

  exit() {
    vars.flinch = false
  }
}
