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
    var land = base_update_air()

    if (done) {
      if (land) {
        fighter.start_action("LandHeavy")
      } else {
        fighter.change_state("Fall")
      }
    }
    // can't land until done
  }

  exit() {
    vars.flinch = false
  }
}
