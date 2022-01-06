import "actions/SmashForward" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    allowAngle = false

    wait_until(5)
    action.play_effect("HorizSmokeA")

    wait_until(9)
    fighter.play_sound("SwingSmashForward")
    fighter.play_sound("SwingExtra")
    if (angle == 0) action.emit_particles("F_Fire")
    else if (angle == -1) action.emit_particles("D_Fire")
    else if (angle == 1) action.emit_particles("U_Fire")
    action.enable_hitblobs("")

    wait_until(12)
    action.disable_hitblobs(true)

    wait_until(42) // 52
    default_end()
  }
}
