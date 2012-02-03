--
-- Luna API core functions
--

--
-- Command helpers
--
function luna.set_topic(channel, newtopic)
    luna.sendline(string.format("TOPIC %s :%s", channel, newtopic))
end

function luna.set_modes(target, modestr, args)
    if args == nil then
        luna.sendline(string.format("MODE %s %s", target, modestr))
    else
        luna.sendline(string.format("MODE %s %s :%s", target, modestr, args))
    end
end

function luna.kick(channel, user, reason)
    if reason == nil then
        reason = user
    end

    luna.sendline(string.format("KICK %s %s :%s", channel, user, reason))
end

function luna.change_nick(new_nick)
    luna.sendline(string.format("NICK :%s", new_nick))
end

function luna.quit(reason)
    if reason == nil then
        reason = "BAI"
    end

    luna.sendline(string.format("QUIT :%s", reason))
end

function luna.part(reason)
    if reason == nil then
        reason = "BAI"
    end

    luna.sendline(string.format("PART :%s", reason))
end

function luna.join_channel(channel, key)
    if key == nil then
        luna.sendline(string.format("JOIN %s", channel))
    else
        luna.sendline(string.format("JOIN %s :%s", channel, key))
    end
end

function luna.notice(target, message)
    luna.sendline(string.format("NOTICE %s :%s", target, message))
end

function luna.privmsg(target, message)
    luna.sendline(string.format("PRIVMSG %s :%s", target, message))
end

function luna.respond(channel, user, response)
    if type(user) == 'table' then
        nickname = user.nick
    else
        nickname = user
    end

    luna.privmsg(channel, string.format("%s: %s", nickname, response))
end

--
-- Add and delete signal handlers
--
function luna.signal_add(sig, handler)
    for i, v in ipairs(luna.__callbacks) do
        for k, v2 in pairs(luna.__callbacks[i]) do
            if v2 == sig then
                error('signal \'' .. v2 .. '\' already in use')
            end
        end
    end

    new_handler = {signal = sig, callback = handler}
    table.insert(luna.__callbacks, new_handler)

    return new_handler
end

function luna.signal_del(handler)
    for i, v in ipairs(luna.__callbacks) do
        if v == handler then
            return table.remove(luna.__callbacks, i)
        end
    end
end

--
-- Register script to Luna
--
function luna.script_register(inf)
    if luna.__scriptinfo ~= nil then
        error('script has already been registered')
    end

    if (type(inf.name) == 'string' and #inf.name ~= 0) and
       (type(inf.author) == 'string' and #inf.author ~= 0) and
       (type(inf.description) == 'string' and #inf.description ~= 0) and
       (type(inf.version) == 'string' and #inf.version ~= 0) then

       luna.__scriptinfo = inf
   else
       error("fields 'name', 'author', 'description' and 'version' are required")
   end
end
