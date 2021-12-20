import "FighterState" for FighterStateScript

class Script is FighterStateScript {
  construct new(s) { super(s) }

  // handler to run upon falling
  doFall=(handler) { _doFall = handler }

  // handler to run upon landing
  doLand=(handler) { _doLand = handler }

  // https://www.ssbwiki.com/B-reversing#B-reverse
  canReverse=(value) { _canReverse = value }

  enter() {
    // specials must set movement vars
    _onGround = vars.onGround
  }

  update() {
    if (_onGround) {
      if (base_update_ground()) {
        if (_doFall is Fn) _doFall.call()
        if (_doFall is String) return _doFall
        _onGround = false
      }
    } else {
      if (base_update_air()) {
        if (_doLand is Fn) _doLand.call()
        if (_doLand is String) return _doLand
        _onGround = true
      }
    }
    if (_canReverse && ctrl.input.relIntX <= -2) {
      fighter.reverse_facing_instant()
      vars.velocity.x = -vars.velocity.x
      _canReverse = false
    }
    // everything else handled by action
  }

  exit() {}
}
