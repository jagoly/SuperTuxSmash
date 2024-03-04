
//========================================================//

foreign class InputFrame {

  foreign pressAttack
  foreign pressSpecial
  foreign pressJump
  foreign pressShield
  foreign pressGrab

  foreign holdAttack
  foreign holdSpecial
  foreign holdJump
  foreign holdShield
  foreign holdGrab

  foreign intX
  foreign intY
  foreign mashX
  foreign mashY
  foreign modX
  foreign modY

  foreign relIntX
  foreign relMashX
  foreign relModX

  foreign floatX
  foreign floatY
}

//========================================================//

foreign class InputHistory {

  foreign iterate(iter)
  foreign iteratorValue(iter)
}

//========================================================//

foreign class Controller {

  foreign history
  foreign input

  foreign clear_history()
}
