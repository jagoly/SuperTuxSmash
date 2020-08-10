function tick()
  action:wait_until(5)
  action:enable_blob_group(0)
  action:wait_until(12)
  action:disable_blob_group(0)
  action:wait_until(30) -- anim 31
  action:allow_interrupt()
end