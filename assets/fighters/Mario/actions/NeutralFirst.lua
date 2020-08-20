function tick()
  action:wait_until(2)
  action:enable_blob_group(0)
  action:wait_until(4)
  action:disable_blob_group(0)
  action:wait_until(15)
  action:allow_interrupt()
end