import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    action.enable_hitblob_group(0)

    wait_until(3)
    action.disable_hitblob_group(0)

    wait_until(10)
    action.play_sound("LandHeavy")

    wait_until(19)
    action.allow_interrupt()
  }
  
  cancel() {
    action.disable_hitblobs()
  }
}
