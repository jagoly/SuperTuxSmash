import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  // set fighter position 1/3 to the ledge
  position_frame_0() {
    vars.position.x = vars.position.x * 0.67 + vars.ledge.position.x * 0.33
    vars.position.y = vars.position.y * 0.67 + vars.ledge.position.y * 0.33
  }

  // set fighter position 2/3 to the ledge
  position_frame_1() {
    vars.position.x = (vars.position.x + vars.ledge.position.x) * 0.5
    vars.position.y = (vars.position.y + vars.ledge.position.y) * 0.5
  }

  // set fighter position to the ledge
  position_frame_2() {
    vars.position.x = vars.ledge.position.x
    vars.position.y = vars.ledge.position.y
  }

  default_begin() {
    fighter.change_state("LedgeHang")
    fighter.play_animation("LedgeCatch", 2, true)
    fighter.set_next_animation("LedgeLoop", 0)
    vars.intangible = true
  }

  default_end() {
    // in brawl, catch and hang are seperate actions
    state.finish_catch()

    // https://www.ssbwiki.com/Edge#Intangibility_duration
    // for now we just use a constant value
    wait_until(48)
    vars.intangible = false

    // seven seconds after catching
    wait_until(336)
  }

  execute() {
    default_begin()
    position_frame_0()

    wait_until(1)
    position_frame_1()

    wait_until(2)
    position_frame_2()

    wait_until(21)
    default_end()
  }

  cancel() {
    vars.intangible = false
  }
}
