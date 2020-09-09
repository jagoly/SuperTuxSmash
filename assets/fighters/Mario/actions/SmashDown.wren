import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(2)
    action.play_sound("SwingExtra")
    action.play_sound("SwingLarge")
    action.enable_hitblob_group(0)

    wait_until(4)
    action.disable_hitblob_group(0)

    wait_until(11)
    action.play_sound("SwingExtra")
    action.play_sound("SwingSmall")
    action.enable_hitblob_group(1)

    wait_until(12)
    action.disable_hitblob_group(1)

    wait_until(35) // 36
    action.allow_interrupt()
  }

  cancel() {
    action.disable_hitblobs()
  }
}
