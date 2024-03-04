
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
  foreign walkAnimSpeed
  foreign dashAnimSpeed
  foreign extraJumps
  foreign lightLandTime
}

//========================================================//

foreign class Variables {

  foreign position
  foreign velocity
  foreign facing
  foreign facing=(value)
  foreign freezeTime
  foreign hitSomething
  foreign animTime
  foreign animTime=(value)
  foreign attachPoint

  foreign bully
  foreign bully=(value)
  foreign victim
  foreign victim=(value)

  foreign extraJumps
  foreign extraJumps=(value)
  foreign lightLandTime
  foreign lightLandTime=(value)
  foreign noCatchTime
  foreign noCatchTime=(value)
  foreign stunTime
  foreign stunTime=(value)

  foreign reboundTime

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

  foreign onGround
  foreign onPlatform

  foreign edge

  foreign moveMobility
  foreign moveMobility=(value)
  foreign moveSpeed
  foreign moveSpeed=(value)

  foreign damage
  foreign shield
  foreign launchSpeed

  foreign ledge
  foreign ledge=(value)
}

//========================================================//

foreign class Fighter {

  foreign ==(other)
  foreign !=(other)

  foreign name
  foreign world

  foreign index

  foreign attributes
  foreign variables
  foreign diamond
  foreign controller
  foreign library

  foreign action
  foreign state

  foreign log(message)

  foreign cxx_assign_action(key)
  foreign cxx_assign_action_null()
  foreign cxx_assign_state(key)

  foreign reverse_facing_auto()
  foreign reverse_facing_instant()
  foreign reverse_facing_slow(clockwise, time)
  foreign reverse_facing_animated(clockwise)

  foreign reset_collisions()

  foreign play_animation(key, fade, fromStart)
  foreign set_next_animation(key, fade)

  foreign play_sound(key, transient)

  foreign attempt_ledge_catch()

  foreign enable_hurtblob(key)
  foreign disable_hurtblob(key)

  foreign cxx_spawn_article(key)

  // spawn an article and call its constructor
  spawn_article(key) {
    var article = cxx_spawn_article(key)
    article.script = article.scriptClass.new(article)
    return article
  }

  // activate an action or call a pseudo action
  start_action(newAction) {
    // todo: put actions and pseudo actions in the same map
    newAction = library.actions[newAction] || newAction

    if (newAction is String) {
      cxx_assign_action(newAction)
      action.do_start()
    }
    else if (newAction is Fn) {
      cxx_assign_action_null()
      newAction.call()
    }
    else Fiber.abort("invalid argument")
  }

  // cancel the active action if there is one
  cancel_action() {
    if (action) {
      action.do_cancel()
      cxx_assign_action_null()
    }
  }

  // exit the active state and enter another
  change_state(newState) {
    state.do_exit()
    cxx_assign_state(newState)
    state.do_enter()
  }
}
