function tick()
  fighter.velocity_x = 0.0
  action:wait_until(6)
  action:enable_blob_group(0)
  action:wait_until(10)
  action:disable_blob_group(0)
  action:enable_blob_group(1)
  action:wait_until(26)
  action:disable_blob_group(1)
  action:wait_until(38) -- anim 54
  action:allow_interrupt()
end