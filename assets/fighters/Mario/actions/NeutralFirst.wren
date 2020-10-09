import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(1)
    action.play_sound("SwingSmall")
    action.enable_hitblobs("")

    wait_until(3)
    action.disable_hitblobs()

    wait_until(6)
    action.set_flag_AllowNext()

    wait_until(9)
    action.set_flag_AutoJab()

    wait_until(15) // 16
    action.allow_interrupt()

    wait_until(23) // no combo
  }

  cancel() {
    action.disable_hitblobs()
  }
}