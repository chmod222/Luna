function connect()
    luna.join_channel('#luna')
end

function cmd(who, channel, command, args)
    if command == 'ping' then
        luna.privmsg(channel, who.nick .. ': pong')
    end
end

luna.script_register{name = 'Base', description = 'Handle basic events',
                     version = '0.1', author = 'FliPPeh'}
luna.signal_add('connect', connect)
luna.signal_add('public_command', cmd)
