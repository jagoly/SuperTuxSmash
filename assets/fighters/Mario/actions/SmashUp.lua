function tick()
  action:wait_until(3)
  --fighter.head_intangible = true
  action:enable_blob_group(0)
  action:wait_until(9)
  --fighter.head_intangible = false
  action:disable_blob_group(0)
  action:wait_until(34) -- anim 34
  action:allow_interrupt()
end