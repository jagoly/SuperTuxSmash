function tick()
    fighter.velocity_x = 0.0
    action:wait_until(32) -- todo: get proper value
    action:finish_action()
end

function cancel() end
