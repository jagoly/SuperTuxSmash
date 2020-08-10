-- frame data for this isn't on smash wiki
-- need to check if this is the same for all chars

function tick()
  fighter.intangible = true
  action:wait_until(23)
  fighter.intangible = false
  action:wait_until(29)
  action:allow_interrupt()
end

function cancel()
  fighter.intangible = false
end