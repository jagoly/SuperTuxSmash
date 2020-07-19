function tick()
    action:spawn_projectile('NeutralOrb')
    action:wait_until(32)
    action:finish_action()
end

function cancel() end
