import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

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

    wait_until(21)
    default_end()
  }

  cancel() {
    vars.intangible = false
  }
}
