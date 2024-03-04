import "actions/GrabbedFree" for Script as Base

class Script is Base {
  construct new(a) { super(a) }

  execute() {
    default_begin()

    wait_until(30)
    default_end()
  }
}
