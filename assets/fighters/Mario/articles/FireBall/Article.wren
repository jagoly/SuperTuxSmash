import "Article" for ArticleScript

class Script is ArticleScript {

  construct new(article) {
    super(article)
    article.fiber = Fiber.new { execute() }

    article.play_animation("Animation", 0, true)

    vars.fragile = true

    vars.position.x = fvars.position.x + fvars.facing * 1.1
    vars.position.y = fvars.position.y + 0.8

    vars.facing = fvars.facing

    vars.velocity.x = 0.114983 * vars.facing
    vars.velocity.y = -0.079868
  }

  execute() {
    article.enable_hitblobs("A")

    for (frame in 1..5) {
      wait_until(frame)
      article.emit_particles("Trail")
    }

    article.disable_hitblobs(false)
    article.enable_hitblobs("B")

    for (frame in 6..30) {
      wait_until(frame)
      article.emit_particles("Trail")
    }

    article.disable_hitblobs(false)
    article.enable_hitblobs("C")

    for (frame in 31..97) {
      wait_until(frame)
      article.emit_particles("Trail")
    }

    wait_until(98)
  }

  update() {
    // apply gravity
    vars.velocity.y = vars.velocity.y - 0.0036

    if (vars.bounced) {
      // todo: Origin should be the point of collision, and the disc
      //       shape should be rotated for walls and ceilings. Particle
      //       system needs an overhaul so will leave it until then.
      article.emit_particles("Bounce")
      article.play_sound("FireBallBounce", false)

      // todo: currently bounce logic is hardcoded
      //       need to allow more control of how each article should move
      var vel = vars.velocity
      var speed = (vel.x * vel.x + vel.y * vel.y).sqrt
      if (speed < 0.08) return true
    }
  }

  destroy() {
    if (vars.hitSomething) article.emit_particles("Hit")
  }
}
