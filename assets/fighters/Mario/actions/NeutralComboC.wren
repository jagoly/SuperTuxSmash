import "actions/GenericAttack" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin("NeutralComboC")

    wait_until(5)
    action.play_sound("Swing3")

    wait_until(6)
    action.enable_hitblobs("")

    wait_until(9)
    action.disable_hitblobs(true)

    wait_until(29) // 40
    default_end()
  }
}
