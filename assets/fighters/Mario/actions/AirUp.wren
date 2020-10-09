import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(1)
    fighter.set_autocancel(false)

    wait_until(3)
    action.play_sound("SwingSmall")
    action.enable_hitblobs("")

    wait_until(9)
    action.disable_hitblobs()

    wait_until(15)
    fighter.set_autocancel(true)

    wait_until(29) // 34
    action.allow_interrupt()
  }
  
  cancel() {
    fighter.set_autocancel(true)
    action.disable_hitblobs()
  }
}
