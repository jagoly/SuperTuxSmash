-- frame data for this isn't on smash wiki
-- need to check if this is the same for all chars

function tick()
  fighter.intangible = true
  action:wait_until(31)
  fighter.intangible = false
  action:wait_until(34)
  action:allow_interrupt()
end

function cancel()
  fighter.intangible = false
end