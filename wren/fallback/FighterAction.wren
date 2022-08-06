import "FighterAction" for FighterActionScript

class Script is FighterActionScript {
  construct new(a) { super(a) }

  execute() {
    return vars.onGround ? "Dodge" : "AirDodge"
  }
}
