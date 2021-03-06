import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(4)
    action.play_sound("SwingMedium")
    action.enable_hitblobs("")

    wait_until(11)
    action.disable_hitblobs()

    wait_until(29) // 31
    action.allow_interrupt()
  }

  cancel() {
    action.disable_hitblobs()
  }
}
