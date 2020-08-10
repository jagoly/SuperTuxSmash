function tick()
  action:wait_until(9)
  action:emit_particles('Fire', 28)
  action:enable_blob_group(0)
  action:wait_until(12)
  action:disable_blob_group(0)
  action:wait_until(43) -- anim 52
  action:allow_interrupt()
end