function tick()
  action:enable_blob_group(0)
  action:wait_until(4)
  action:disable_blob_group(0)
  action:wait_until(19)
  action:allow_interrupt()
end