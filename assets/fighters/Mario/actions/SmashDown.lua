function tick()
  action:wait_until(3)
  action:enable_blob_group(0)
  action:wait_until(5)
  action:disable_blob_group(0)
  action:wait_until(12)
  action:enable_blob_group(1)
  action:wait_until(13)
  action:disable_blob_group(1)
  action:wait_until(36) -- anim 36
  action:allow_interrupt()
end