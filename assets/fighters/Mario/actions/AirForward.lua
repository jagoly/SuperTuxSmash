function tick()
  action:wait_until(3)
  fighter.autocancel = false
  action:wait_until(16)
  action:enable_blob_group(0)
  action:wait_until(17)
  action:disable_blob_group(0)
  action:enable_blob_group(1)
  action:wait_until(20)
  action:disable_blob_group(1)
  action:enable_blob_group(2)
  action:wait_until(22)
  action:disable_blob_group(2)
  action:wait_until(43)
  fighter.autocancel = true
  action:wait_until(60)
  action:allow_interrupt()
end

function cancel()
  fighter.autocancel = true
end