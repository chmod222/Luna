function connect()
    luna.join_channel('#luna')
end

function cmd(who, channel, command, args)
    if command == 'ping' then
        luna.privmsg(channel, who.nick .. ': pong')
    end
end

luna.script_register('Base', 'Handle basic events', '0.1', 'FliPPeh')
luna.signal_add('connect', connect)
luna.signal_add('public_command', cmd)
