//========================================================//

foreign class Action {

  foreign set_wait_until(frame)
  foreign allow_interrupt()
  foreign enable_hitblob_group(group)
  foreign disable_hitblob_group(group)
  foreign disable_hitblobs()
  foreign emit_particles(key)
  foreign play_sound(key)
}

//========================================================//

foreign class Fighter {
  
  foreign set_intangible(value)
  foreign enable_hurtblob(key)
  foreign disable_hurtblob(key)
  foreign set_velocity_x(value)
  foreign set_autocancel(value)
}

//========================================================//

class ScriptBase {

  construct new(action, fighter) {
    _action = action
    _fighter = fighter
  }

  reset() { Fiber.new { execute() } }
  
  wait_until(frame) {
    action.set_wait_until(frame)
    Fiber.yield()
  }

  action { _action }
  fighter { _fighter }
}
