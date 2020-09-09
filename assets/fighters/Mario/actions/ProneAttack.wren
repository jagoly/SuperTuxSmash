import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {
    fighter.set_intangible(true)

    wait_until(18)
    action.play_sound("SwingLarge")
    action.enable_hitblob_group(0)

    wait_until(20)
    action.disable_hitblob_group(0)

    wait_until(24)
    action.play_sound("SwingLarge")
    action.enable_hitblob_group(1)

    wait_until(26)
    fighter.set_intangible(false)
    action.disable_hitblob_group(1)

    wait_until(48)
    action.allow_interrupt()
  }

  cancel() {
    fighter.set_intangible(false)
    action.disable_hitblobs()
  }
}
