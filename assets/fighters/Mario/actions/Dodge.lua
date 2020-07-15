function tick()
    action:wait_until(2)
    fighter.intangible = true
    action:wait_until(15)
    fighter.intangible = false
    action:wait_until(22)
    action:finish_action()
end

function cancel()
    fighter.intangible = false
end
