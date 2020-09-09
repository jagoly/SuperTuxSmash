import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(1)
    action.play_sound("SwingSmall")
    action.enable_hitblob_group(0)

    wait_until(3)
    action.disable_hitblob_group(0)

    wait_until(15) // 16
    action.allow_interrupt()
  }

  cancel() {
    action.disable_hitblobs()
  }
}
