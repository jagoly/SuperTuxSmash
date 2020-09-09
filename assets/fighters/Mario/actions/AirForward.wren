import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(2)
    fighter.set_autocancel(false)

    wait_until(15)
    action.play_sound("SwingSmall")
    action.enable_hitblob_group(0)

    wait_until(16)
    action.disable_hitblob_group(0)
    action.enable_hitblob_group(1)

    wait_until(19)
    action.disable_hitblob_group(1)
    action.enable_hitblob_group(2)

    wait_until(21)
    action.disable_hitblob_group(2)

    wait_until(42)
    fighter.set_autocancel(true)

    wait_until(59) // 75
    action.allow_interrupt()
  }
  
  cancel() {
    fighter.set_autocancel(true)
    action.disable_hitblobs()
  }
}
