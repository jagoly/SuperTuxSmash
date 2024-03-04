
//========================================================//

foreign class FighterState {

  foreign name
  foreign fighter
  foreign world
  foreign script

  foreign log_with_prefix(message)

  foreign cxx_before_enter()
  foreign cxx_before_exit()

  do_enter() {
    cxx_before_enter()
    script.enter()
  }

  do_updates() {
    var newAction = script.update()
    if (newAction) {
      log_with_prefix("update  -> %(newAction)")
      if (fighter.action) {
        fighter.action.do_cancel()
      }
      fighter.start_action(newAction)
    }
  }

  do_exit() {
    cxx_before_exit()
    script.exit()
  }
}

//========================================================//

class FighterStateScript {

  construct new(state) {
    _state = state
  }

  state { _state }
  fighter { _state.fighter }
  world { _state.world }

  attrs { _state.fighter.attributes }
  vars { _state.fighter.variables }
  dmnd { _state.fighter.diamond }
  ctrl { _state.fighter.controller }
  lib { _state.fighter.library }

  //--------------------------------------------------------//

  // called each update by most ground states
  // returns true if the fighter is no longer on the ground
  base_update_ground() {

    if (vars.onGround == true) {

        // reset no ledge catch timer
        vars.noCatchTime = 48

        return false
    }

    return true
  }

  // called each update by most air states
  // returns true if the fighter is no longer in the air
  base_update_air() {

    if (vars.onGround == false) {

      // decrement no ledge catch timer
      if (vars.noCatchTime != 0) {
        vars.noCatchTime = vars.noCatchTime - 1
      }

      // reset or decrement light landing timer
      if (vars.velocity.y > -attrs.fallSpeed) {
        vars.lightLandTime = attrs.lightLandTime
      }
      else if (vars.lightLandTime != 0) {
        vars.lightLandTime = vars.lightLandTime - 1
      }

      return false
    }

    return true
  }

  // called each update by stun states
  // returns true if the fighter is no longer stunned
  base_update_stun() {
    vars.stunTime = vars.stunTime - 1
    return vars.stunTime == 0
  }
}
