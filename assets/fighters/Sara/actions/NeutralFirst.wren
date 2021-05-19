import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    wait_until(2)
    //action.play_sound("SwingSmall")
    action.enable_hitblobs("")

    wait_until(4)
    action.disable_hitblobs()

    wait_until(8)
    action.set_flag_AllowNext()

    wait_until(11)
    action.set_flag_AutoJab()

    wait_until(14) // 15
    action.allow_interrupt()

    wait_until(23)
  }

  cancel() {
    action.disable_hitblobs()
  }
}
