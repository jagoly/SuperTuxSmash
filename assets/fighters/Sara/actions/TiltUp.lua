function tick()
    action:wait_until(4)
    action:enable_blob('Tangy')
    action:emit_particles('Tangy', 20)
    action:wait_until(7)
    action:enable_blob('SourA')
    action:enable_blob('SourB')
    action:emit_particles('SourA', 16)
    action:emit_particles('SourB', 16)
    action:disable_blob('Tangy')
    action:wait_until(13)
    action:disable_blob('SourA')
    action:disable_blob('SourB')
    action:wait_until(18)
    action:finish_action()
end

function cancel() end
