
//========================================================//

class FighterLibrary {

  construct new(fighter) {

    _fighter = fighter
    _actions = {}

    define_pseudo_actions()
  }

  fighter { _fighter }
  world { _fighter.world }

  attrs { _fighter.attributes }
  vars { _fighter.variables }
  dmnd { _fighter.diamond }
  ctrl { _fighter.controller }

  state { _fighter.state.script }

  actions { _actions }

  //--------------------------------------------------------//

  define_pseudo_actions() {

    // from Walk or Vertigo, return to Neutral
    actions["MiscNeutral"] = Fn.new {
      fighter.change_state("Neutral")
      fighter.play_animation("NeutralLoop", 4, true)
    }

    // from various ground states, change to Fall
    actions["MiscFall"] = Fn.new {
      fighter.change_state("Fall")
      fighter.play_animation("FallLoop", 4, true)
    }

    // from various ground states, change to Tumble
    actions["MiscTumble"] = Fn.new {
      fighter.change_state("Tumble")
      fighter.play_animation("TumbleLoop", 4, true)
    }

    // from Neutral, change to Walk
    actions["Walk"] = Fn.new {
      fighter.change_state("Walk")
      fighter.play_animation("WalkLoop", 4, true)
    }

    // from BrakeTurn, return to Neutral
    actions["BrakeTurnStop"] = Fn.new {
      fighter.change_state("Neutral")
      fighter.play_animation("BrakeTurnStop", 2, true)
      fighter.set_next_animation("NeutralLoop", 0)
    }

    // from Neutral, drop through a platform
    actions["PlatformDrop"] = Fn.new {
      // todo: play the actual platform drop animation
      ctrl.clear_history()
      fighter.change_state("Fall")
      fighter.play_animation("FallLoop", 4, true)
      vars.position.y = vars.position.y - 0.1
    }

    // from LedgeHang, drop from the ledge
    actions["LedgeDrop"] = Fn.new {
      fighter.change_state("Fall")
      fighter.play_animation("FallLoop", 2, true)
    }

    // from DashStart, change to Dash
    actions["Dash"] = Fn.new {
      fighter.change_state("Dash")
      fighter.play_animation("DashLoop", 1, true)
    }

    // choose either a heavy or light landing
    actions["MiscLand"] = Fn.new {
      if (vars.fastFall || vars.lightLandTime == 0) {
        fighter.start_action("LandHeavy")
      } else {
        fighter.start_action("LandLight")
      }
    }

    // start fast falling
    actions["FastFall"] = Fn.new {
      // todo: make some sparkles
      vars.fastFall = true
    }

    // not implemented yet
    actions["ShieldBreak"] = Fn.new {
      fighter.start_action("LandTumble")
    }

    // from Grab, change to Fall
    actions["GrabFall"] = Fn.new {
      fighter.change_state("Fall")
      fighter.play_animation("GrabFree", 2, true)
      fighter.set_next_animation("FallLoop", 4)
    }

    // from Grabbed, change to Fall
    actions["GrabbedFall"] = Fn.new {
      fighter.change_state("Fall")
      fighter.play_animation("GrabbedFreeLow", 2, true)
      fighter.set_next_animation("FallLoop", 4)
    }
  }

  //--------------------------------------------------------//

  check_GroundDodges(frame) {
    if (frame.pressShield) {
      // prefer neg Y
      if (frame.intX.abs > -frame.intY || frame.modY >= 0) {
        if (frame.relModX == 1) return "EvadeForward"
        if (frame.relModX == -1) return "EvadeBack"
      }
      if (frame.modY == -1) return "Dodge"
    }
  }

  check_GroundDodgesRv(frame) {
    if (frame.pressShield) {
      // prefer neg Y
      if (frame.intX.abs > -frame.intY || frame.modY >= 0) {
        if (frame.relModX == 1) {
          fighter.reverse_facing_auto()
          return "EvadeBack"
        }
        if (frame.relModX == -1) {
          fighter.reverse_facing_auto()
          return "EvadeForward"
        }
      }
      if (frame.modY == -1) return "Dodge"
    }
  }

  check_GroundSpecials(frame) {
    if (frame.pressSpecial) {
      // prefer Y
      if (frame.intX.abs > frame.intY.abs) {
        if (frame.relIntX >= 3) return "SpecialForward"
        if (frame.relIntX <= -3) return "SpecialForwardRv"
      }
      else if (frame.intY >= 3) {
        return frame.relIntX >= 0 ? "SpecialUp" : "SpecialUpRv"
      }
      else if (frame.intY <= -3) {
        return frame.relIntX >= 0 ? "SpecialDown" : "SpecialDownRv"
      }
      return "SpecialNeutral"
    }
  }

  check_AirSpecials(frame) {
    if (frame.pressSpecial) {
      // prefer Y
      if (frame.intX.abs > frame.intY.abs) {
        if (frame.relIntX >= 3) return "SpecialAirForward"
        if (frame.relIntX <= -3) return "SpecialAirForwardRv"
      }
      else if (frame.intY >= 3) {
        return frame.relIntX >= 0 ? "SpecialAirUp" : "SpecialAirUpRv"
      }
      else if (frame.intY <= -3) {
        return frame.relIntX >= 0 ? "SpecialAirDown" : "SpecialAirDownRv"
      }
      return "SpecialAirNeutral"
    }
  }

  check_GroundSmashes(frame) {
    if (frame.pressAttack) {
      // prefer X
      if (frame.intX.abs >= frame.intY.abs) {
        if (frame.relModX == 1) return "ChargeForward"
        if (frame.relModX == -1) return "ChargeForwardRv"
      }
      else if (frame.modY == 1) return "ChargeUp"
      else if (frame.modY == -1) return "ChargeDown"
    }
  }

  check_GroundAttacks(frame) {
    if (frame.pressAttack) {
      // prefer X
      if (frame.intX.abs >= frame.intY.abs) {
        if (frame.relIntX >= 1) return "TiltForward"
        if (frame.relIntX <= -1) return "TiltForwardRv"
      }
      else if (frame.intY >= 1) return "TiltUp"
      else if (frame.intY <= -1) return "TiltDown"
      return "NeutralComboA"
    }
  }

  check_AirAttacks(frame) {
    if (frame.pressAttack) {
      // prefer X
      if (frame.intX.abs >= frame.intY.abs) {
        if (frame.relIntX >= 1) return "AirForward"
        if (frame.relIntX <= -1) return "AirBack"
      }
      else if (frame.intY >= 1) return "AirUp"
      else if (frame.intY <= -1) return "AirDown"
      return "AirNeutral"
    }
  }

  check_AirHops(frame) {
    if (frame.pressJump && vars.extraJumps > 0) {
      return frame.relIntX >= 0 ? "AirHopForward" : "AirHopBack"
    }
  }

  check_DashStart(frame) {
    if (frame.intY.abs < 3) {
      if (frame.relMashX == 1) return "DashStart"
      if (frame.relMashX == -1) return "DashStartTurn"
    }
  }

  //--------------------------------------------------------//

  // calculate the initial x velocity for a jump
  assign_jump_velocity_x() {
    vars.velocity.x =
      ((vars.velocity.x + attrs.airSpeed * ctrl.input.floatX) * 0.5).
      clamp(-attrs.airSpeed, attrs.airSpeed)
  }

  // calculate the initial y velocity for a jump
  assign_jump_velocity_y(height) {
    vars.velocity.y =
      (2.0 * height * attrs.gravity).sqrt + attrs.gravity * 0.5
  }
}
