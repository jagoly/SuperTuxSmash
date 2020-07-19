function tick()
    print(foo)
    foo = 'down'
    action:wait_until(4)
    action:enable_blob('BlobA')
    action:emit_particles('PuffA', 40)
    action:wait_until(8)
    action:disable_blob('BlobA')
    action:enable_blob('BlobB')
    action:emit_particles('PuffB', 40)
    action:wait_until(12)
    action:disable_blob('BlobB')
    action:enable_blob('BlobC')
    action:emit_particles('PuffC', 50)
    action:wait_until(16)
    action:disable_blob('BlobC')
    action:wait_until(28)
    action:finish_action()
end

function cancel() end
