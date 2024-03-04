
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

  foreign fragile
  foreign fragile=(value)
  foreign bounced
}

//========================================================//

foreign class Article {

  foreign name
  foreign world

  foreign fighter
  foreign variables
  foreign scriptClass

  foreign script
  foreign script=(value)

  foreign fiber
  foreign fiber=(value)

  foreign log_with_prefix(message)

  foreign cxx_wait_until(frame)
  foreign cxx_wait_for(frame)
  foreign cxx_next_frame()

  foreign mark_for_destroy()

  foreign reverse_facing_auto()
  foreign reverse_facing_instant()
  foreign reverse_facing_slow(clockwise, time)
  foreign reverse_facing_animated(clockwise)

  foreign play_animation(key, fade, fromStart)
  foreign set_next_animation(key, fade)

  foreign play_sound(key, stopWithAction)

  foreign enable_hitblobs(prefix)
  foreign disable_hitblobs(resetCollisions)
  foreign play_effect(key)
  foreign emit_particles(key)

  do_updates() {
    if (cxx_next_frame()) {
      fiber.call()
      if (fiber.isDone) {
        mark_for_destroy()
        return // don't call update
      }
    }
    if (script.update()) {
      mark_for_destroy()
    }
  }

  do_destroy() {
    log_with_prefix("destroy %(name)")
    script.destroy()
  }
}

//========================================================//

class ArticleScript {

  construct new(article) {
    _article = article
    _article.log_with_prefix("create  %(_article.name)")
  }

  article { _article }
  fighter { _article.fighter }
  world { _article.world }

  vars { _article.variables }

  fattrs { _article.fighter.attributes }
  fvars { _article.fighter.variables }
  fdmnd { _article.fighter.diamond }
  fctrl { _article.fighter.controller }
  flib { _article.fighter.library }

  // suspend fiber until the specified frame
  wait_until(frame) {
    _article.cxx_wait_until(frame)
    Fiber.yield()
  }

  // suspend fiber for the specified number of frames
  wait_for(frames) {
    _article.cxx_wait_for(frames)
    Fiber.yield()
  }

  // optional method, called every frame
  update() {}

  // optional method, called before destruction
  destroy() {}
}
