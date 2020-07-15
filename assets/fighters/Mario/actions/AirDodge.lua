function tick()
    action:wait_until(4)
    fighter.intangible = true
    action:wait_until(29)
    fighter.intangible = false
    action:wait_until(49)
    action:finish_action()
end

function cancel()
    fighter.intangible = false
end
