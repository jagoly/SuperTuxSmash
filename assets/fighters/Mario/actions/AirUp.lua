function tick()
  action:wait_until(2)
  fighter.autocancel = false
  action:wait_until(4)
  action:enable_blob_group(0)
  action:wait_until(10)
  action:disable_blob_group(0)
  action:wait_until(16)
  fighter.autocancel = true
  action:wait_until(30)
  action:allow_interrupt()
end

function cancel()
  fighter.autocancel = true
end