function tick()
    action:wait_until(5)
    action:enable_blob('BlobA')
    action:enable_blob('BlobB')
    action:enable_blob('BlobC')
    action:wait_until(14)
    action:disable_blob('BlobA')
    action:disable_blob('BlobB')
    action:disable_blob('BlobC')
    action:wait_until(22)
    action:finish_action()
end

function cancel() end
