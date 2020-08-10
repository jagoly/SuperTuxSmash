function tick()
  action:wait_until(4)
  fighter.intangible = true
  action:wait_until(20)
  fighter.intangible = false
  action:wait_until(33)
  action:allow_interrupt()
end

function cancel()
  fighter.intangible = false
end