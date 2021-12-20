import "actions/DashAttack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(5)
    action.enable_hitblobs("CLEAN")

    wait_until(6)
    action.play_sound("SwingMedium")

    wait_until(9)
    action.disable_hitblobs(false)
    action.enable_hitblobs("LATE")

    wait_until(25)
    action.disable_hitblobs(true)

    wait_until(37) // 54
    default_end()
  }
}
