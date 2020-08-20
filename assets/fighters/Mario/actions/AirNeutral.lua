function tick()
  action:wait_until(3)
  fighter.autocancel = false
  action:enable_blob_group(0)
  action:wait_until(6)
  action:disable_blob_group(0)
  action:enable_blob_group(1)
  action:wait_until(30)
  action:disable_blob_group(1)
  action:wait_until(34)
  fighter.autocancel = true
  action:wait_until(45)
  action:allow_interrupt()
end

function cancel()
  fighter.autocancel = true
end