function tick()
  action:wait_until(4)
  fighter.intangible = true
  action:wait_until(30)
  fighter.intangible = false
  action:wait_until(49)
  action:allow_interrupt()
end

function cancel()
  fighter.intangible = false
end