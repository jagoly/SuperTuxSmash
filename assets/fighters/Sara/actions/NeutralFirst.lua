function tick()
    action:wait_until(2)
    action:enable_blob('BlobA')
    action:wait_until(4)
    action:disable_blob('BlobA')
    action:enable_blob('BlobB')
    action:wait_until(6)
    action:disable_blob('BlobB')
    action:wait_until(14)
    action:finish_action()
end

function cancel() end
