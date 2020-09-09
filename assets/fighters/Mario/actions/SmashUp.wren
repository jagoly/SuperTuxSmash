import "sts" for ScriptBase

class Script is ScriptBase {
  construct new(a, f) { super(a, f) }

  execute() {

    wait_until(2)
    fighter.disable_hurtblob("HeadN")
    action.play_sound("SwingExtra")
    action.play_sound("SwingLarge")
    action.enable_hitblob_group(0)

    wait_until(8)
    fighter.enable_hurtblob("HeadN")
    action.disable_hitblob_group(0)

    wait_until(33)
    action.allow_interrupt()
  }

  cancel() {
    fighter.enable_hurtblob("HeadN")
    action.disable_hitblobs()
  }
}
