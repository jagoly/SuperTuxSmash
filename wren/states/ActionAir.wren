import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // handler to run upon landing
  doLand=(handler) { _doLand = handler }

  enter() {
    // defaults (actions may override)
    vars.moveMobility = attrs.airMobility
    vars.moveSpeed = attrs.airSpeed
  }

  update() {
    if (base_update_air()) {
      if (_doLand is String) return _doLand
      if (_doLand is Fn) return _doLand.call()
    }
    // everything else handled by action
  }

  exit() {}
}
