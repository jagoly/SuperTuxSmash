import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // handler to run upon falling
  doFall=(handler) { _doFall = handler }

  enter() {
    // defaults (actions may override)
    vars.edgeStop = "Always"
    vars.moveMobility = 0.0
  }

  update() {
    if (base_update_ground()) {
      if (_doFall is String) return _doFall
      if (_doFall is Fn) return _doFall.call()
    }
    // everything else handled by action
  }

  exit() {}
}
