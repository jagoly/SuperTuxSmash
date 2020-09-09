import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(5)
    fighter.set_autocancel(false)
    action.play_sound("SwingSmall")
    action.enable_hitblob_group(0)

    wait_until(8)
    action.disable_hitblob_group(0)
    action.enable_hitblob_group(1)

    wait_until(13)
    action.disable_hitblob_group(1)

    wait_until(18)
    fighter.set_autocancel(true)

    wait_until(33) // 46
    action.allow_interrupt()
  }
  
  cancel() {
    fighter.set_autocancel(true)
    action.disable_hitblobs()
  }
}
