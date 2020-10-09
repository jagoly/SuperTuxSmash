import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(2)
    fighter.set_autocancel(false)
    action.play_sound("SwingSmall")
    action.enable_hitblobs("CLEAN")

    wait_until(5)
    action.disable_hitblobs()
    action.enable_hitblobs("LATE")

    wait_until(29)
    action.disable_hitblobs()

    wait_until(33)
    fighter.set_autocancel(true)

    wait_until(45) // 46
    action.allow_interrupt()
  }
  
  cancel() {
    fighter.set_autocancel(true)
    action.disable_hitblobs()
  }
}
