function tick()
    action:wait_until(4)
    action:enable_blob("Hip")
    action:enable_blob("Leg")
    action:enable_blob("Knee")
    action:wait_until(8)
    action:disable_blob("Hip")
    action:disable_blob("Leg")
    action:disable_blob("Knee")
    action:wait_until(28) -- anim = 34
    action:finish_action()
end

function cancel() end
