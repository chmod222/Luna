luna.registerScript{name    = 'Base', description = 'Handle basic events',
                    version = '0.1',  author      = 'FliPPeh'}

luna.addSignal('connect', function()
    luna.joinChannel('#luna')
end)

luna.addSignal('public_command', function(who, where, cmd, args)
    if cmd == 'ping' then
        who:respond('pong')
    end
end)
