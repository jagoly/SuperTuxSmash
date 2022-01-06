import "actions/LedgeJump" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    fighter.play_sound("DashStart")

    wait_until(15)
    set_vertical_velocity()

    wait_until(16) // 16
    set_horizontal_velocity()
    default_end()

    wait_until(17)
    fighter.play_sound("DashStart")

    wait_until(33)
    fighter.play_sound("Whoosh2")
  }
}
