function tick()
  action:wait_until(5)
  action:enable_blob_group(0)
  action:wait_until(8)
  action:disable_blob_group(0)
  action:wait_until(35) -- anim 35
  action:allow_interrupt()
end