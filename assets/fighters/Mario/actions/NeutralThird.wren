import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(5)
    action.play_sound("SwingLarge")

    wait_until(6)
    action.enable_hitblobs("")

    wait_until(9)
    action.disable_hitblobs()

    wait_until(29) // 40
    action.allow_interrupt()
  }

  cancel() {
    action.disable_hitblobs()
  }
}