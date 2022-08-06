import "FighterLibrary" for FighterLibrary

class Library is FighterLibrary {

  construct new(fighter) {
    super(fighter)
  }

  define_pseudo_actions() {
    super.define_pseudo_actions()

    // down special
    actions["SpecialAirDown"] = Fn.new {
      fighter.start_action("SpecialDown")
    }
    actions["SpecialDownRv"] = Fn.new {
      fighter.start_action("SpecialDown")
    }
    actions["SpecialAirDownRv"] = Fn.new {
      fighter.start_action("SpecialDown")
    }

    // forward special
    actions["SpecialAirForward"] = Fn.new {
      fighter.start_action("SpecialForward")
    }
    actions["SpecialForwardRv"] = Fn.new {
      fighter.reverse_facing_instant()
      fighter.start_action("SpecialForward")
    }
    actions["SpecialAirForwardRv"] = Fn.new {
      fighter.reverse_facing_instant()
      fighter.start_action("SpecialForward")
    }

    // neutral special
    actions["SpecialAirNeutral"] = Fn.new {
      fighter.start_action("SpecialNeutral")
    }
    actions["SpecialNeutralRv"] = Fn.new {
      fighter.reverse_facing_instant()
      fighter.start_action("SpecialNeutral")
    }
    actions["SpecialAirNeutralRv"] = Fn.new {
      fighter.reverse_facing_instant()
      fighter.start_action("SpecialNeutral")
    }

    // up special
    actions["SpecialAirUp"] = Fn.new {
      fighter.start_action("SpecialUp")
    }
    actions["SpecialUpRv"] = Fn.new {
      fighter.reverse_facing_instant()
      fighter.start_action("SpecialUp")
    }
    actions["SpecialAirUpRv"] = Fn.new {
      fighter.reverse_facing_instant()
      fighter.start_action("SpecialUp")
    }

    // reverse smash
    actions["ChargeForwardRv"] = Fn.new {
      fighter.reverse_facing_slow(true, 5)
      fighter.start_action("ChargeForward")
    }

    // reverse tilt
    actions["TiltForwardRv"] = Fn.new {
      fighter.reverse_facing_slow(false, 3)
      fighter.start_action("TiltForward")
    }
  }

  play_random_voice_attack() {
    var index = world.random_int(0, 3)
    if (index == 0) fighter.play_sound("VoiceAttackA", false)
    else if (index == 1) fighter.play_sound("VoiceAttackB", false)
    else if (index == 2) fighter.play_sound("VoiceAttackC", false)
    else fighter.play_sound("VoiceAttackD", false)
  }
}
