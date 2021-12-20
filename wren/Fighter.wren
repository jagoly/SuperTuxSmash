
//========================================================//

foreign class Attributes {

  foreign walkSpeed
  foreign dashSpeed
  foreign airSpeed
  foreign traction
  foreign airMobility
  foreign airFriction
  foreign hopHeight
  foreign jumpHeight
  foreign airHopHeight
  foreign gravity
  foreign fallSpeed
  foreign fastFallSpeed
  foreign weight
  foreign extraJumps
  foreign lightLandTime
}

//========================================================//

foreign class Variables {

  foreign position
  foreign velocity
  foreign facing

  foreign extraJumps
  foreign extraJumps=(value)
  foreign lightLandTime
  foreign lightLandTime=(value)
  foreign noCatchTime
  foreign noCatchTime=(value)
  foreign stunTime
  foreign stunTime=(value)
  foreign freezeTime
  foreign freezeTime=(value)
  foreign edgeStop
  foreign edgeStop=(value)
  foreign intangible
  foreign intangible=(value)
  foreign fastFall
  foreign fastFall=(value)
  foreign applyGravity
  foreign applyGravity=(value)
  foreign applyFriction
  foreign applyFriction=(value)
  foreign flinch
  foreign flinch=(value)
  foreign moveMobility
  foreign moveMobility=(value)
  foreign moveSpeed
  foreign moveSpeed=(value)

  foreign vertigo
  foreign onGround
  foreign onPlatform

  foreign ledge
  foreign ledge=(value)
}

//========================================================//

foreign class Fighter {

  foreign ==(other)
  foreign !=(other)

  foreign name
  foreign index

  foreign attributes
  foreign localDiamond
  foreign variables
  foreign controller
  foreign library

  foreign action
  foreign state

  foreign log(message)

  foreign cxx_clear_action()
  foreign cxx_assign_action(key)
  foreign cxx_assign_state(key)

  foreign reverse_facing_auto()
  foreign reverse_facing_instant()
  foreign reverse_facing_slow(clockwise, time)
  foreign reverse_facing_animated(clockwise)

  foreign attempt_ledge_catch()

  foreign play_animation(key, fade, fromStart)
  foreign set_next_animation(key, fade)

  foreign reset_collisions()
  foreign enable_hurtblob(key)
  foreign disable_hurtblob(key)

  // activate an action or call a pseudo action
  start_action(newAction) {
    // todo: put actions and pseudo actions in the same map
    newAction = library.actions[newAction] || newAction

    if (newAction is String) {
      cxx_assign_action(newAction)
      action.do_start()
    }
    else if (newAction is Fn) {
      cxx_clear_action()
      newAction.call()
    }
    else Fiber.abort("invalid argument")
  }

  // cancel the active action if there is one
  cancel_action() {
    if (action) {
      action.do_cancel()
      cxx_clear_action()
    }
  }

  // exit the active state and enter another
  change_state(newState) {
    state.do_exit()
    cxx_assign_state(newState)
    state.do_enter()
  }
}
