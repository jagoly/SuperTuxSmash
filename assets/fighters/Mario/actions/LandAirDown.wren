import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    action.enable_hitblobs("")

    wait_until(3)
    action.disable_hitblobs()

    wait_until(10)
    action.play_sound("LandHeavy")

    wait_until(19) // 31
    action.allow_interrupt()
  }
  
  cancel() {
    action.disable_hitblobs()
  }
}
