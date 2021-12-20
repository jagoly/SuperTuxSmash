import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  enter() {
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
        fighter.start_action("LandTumble")
      } else {
        fighter.change_state("Tumble")
      }
    }
    else if (land) {
      // todo: bounce if launched
      return "LandTumble"
    }
  }

  exit() {
    vars.flinch = false
  }
}
