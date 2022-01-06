
//========================================================//

foreign class FighterAction {

  foreign name
  foreign fighter
  foreign world
  foreign script

  foreign fiber
  foreign fiber=(value)

  foreign log_with_prefix(message)

  foreign cxx_before_start()
  foreign cxx_wait_until(frame)
  foreign cxx_wait_for(frame)
  foreign cxx_next_frame()
  foreign cxx_before_cancel()

  foreign check_hit_something()
  foreign enable_hitblobs(prefix)
  foreign disable_hitblobs(resetCollisions)
  foreign emit_particles(key)
  foreign play_effect(key)

  do_start() {
    cxx_before_start()
    fiber = Fiber.new { script.execute() }
    do_updates()
  }

  do_updates() {
    if (cxx_next_frame()) {
      var newAction = fiber.call()
      if (fiber.isDone) {
        log_with_prefix("execute -> %(newAction)")
        if (newAction) {
          fighter.start_action(newAction)
        } else {
          fighter.cxx_clear_action()
        }
        return // don't call script.update()
      }
    }
    var newAction = script.update()
    if (newAction) {
      log_with_prefix("update  -> %(newAction)")
      do_cancel()
      fighter.start_action(newAction)
    }
  }

  do_cancel() {
    cxx_before_cancel()
    script.cancel()
  }
}

//========================================================//

class FighterActionScript {

  construct new(action) {
    _action = action
  }

  action { _action }
  fighter { _action.fighter }
  world { _action.world }

  attrs { _action.fighter.attributes }
  diamond { _action.fighter.localDiamond }
  vars { _action.fighter.variables }
  ctrl { _action.fighter.controller }
  lib { _action.fighter.library }

  state { _action.fighter.state.script }

  // suspend fiber until the specified frame
  wait_until(frame) {
    _action.cxx_wait_until(frame)
    Fiber.yield()
  }

  // suspend fiber for the specified number of frames
  wait_for(frames) {
    _action.cxx_wait_for(frames)
    Fiber.yield()
  }

  // optional method, called every frame while active
  update() {}

  // optional method, called if action ends abnormally
  cancel() {}
}
