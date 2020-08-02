function tick()
    fighter.velocity_x = 0.0
    -- just copied this from sara, should see what brawl does
    for frame = 1, 11 do
        action:emit_particles('FeetSlide', 12 - frame)
        action:wait_until(frame)
    end
    action:wait_until(32) -- todo: get proper value
    action:finish_action()
end

function cancel() end
