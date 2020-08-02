function tick()
    -- dash attack can use root motion, but this is also possible
    fighter.velocity_x = 0.15 * fighter.facing
    for frame = 1, 11 do
        action:emit_particles('FeetSlide', 12 - frame)
        action:wait_until(frame)
    end
    action:wait_until(32)
    action:finish_action()
end

function cancel() end
