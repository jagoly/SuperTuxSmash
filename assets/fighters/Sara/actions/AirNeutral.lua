function tick()
    -- print frame every 8 frames
    for frame = 8, 72, 8 do
        print(frame)
        action:wait_until(frame)
    end
    action:finish_action()
end

function cancel() end
