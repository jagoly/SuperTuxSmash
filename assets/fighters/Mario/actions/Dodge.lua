function tick()
  action:wait_until(2)
  fighter.intangible = true
  action:wait_until(21)
  fighter.intangible = false
  action:wait_until(26)
  action:allow_interrupt()
end

function cancel()
  fighter.intangible = false
end