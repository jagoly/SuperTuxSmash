function tick()
    action:wait_until(4)
    fighter.intangible = true
    action:wait_until(20)
    fighter.intangible = false
    action:wait_until(32)
    action:finish_action()
end

function cancel()
    fighter.intangible = false
end