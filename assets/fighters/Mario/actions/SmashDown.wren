import "actions/SmashDown" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(2)
    action.play_sound("SwingExtra")
    action.play_sound("SwingLarge")
    action.enable_hitblobs("A")

    wait_until(4)
    action.disable_hitblobs(true)

    wait_until(11)
    action.play_sound("SwingExtra")
    action.play_sound("SwingSmall")
    action.enable_hitblobs("B")

    wait_until(12)
    action.disable_hitblobs(true)

    wait_until(36) // 36
    default_end()
  }
}
