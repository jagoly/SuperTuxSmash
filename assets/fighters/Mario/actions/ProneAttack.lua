-- frame data for this isn't on smash wiki
-- need to check if this is the same for all chars

function tick()
  fighter.intangible = true
  action:wait_until(19)
  action:enable_blob_group(0)
  action:wait_until(21)
  action:disable_blob_group(0)
  action:wait_until(25)
  action:enable_blob_group(1)
  action:wait_until(27)
  action:disable_blob_group(1)
  fighter.intangible = false
  action:wait_until(48)
  action:allow_interrupt()
end