--
-- Luna API core functions
--

--
-- Command helpers
--
function luna.changeNick(new_nick)
    luna.sendLine(string.format("NICK :%s", new_nick))
end

function luna.quit(reason)
    if reason == nil then
        reason = "BAI"
    end

    luna.sendLine(string.format("QUIT :%s", reason))
end

function luna.joinChannel(channel, key)
    if key == nil then
        luna.sendLine(string.format("JOIN %s", channel))
    else
        luna.sendLine(string.format("JOIN %s :%s", channel, key))
    end
end

function luna.notice(target, message)
    luna.sendLine(string.format("NOTICE %s :%s", target, message))
end

function luna.privmsg(target, message)
    luna.sendLine(string.format("PRIVMSG %s :%s", target, message))
end

--
-- Add and delete signal handlers
--
function luna.addSignal(sig, handler)
    for i, v in ipairs(luna.__callbacks) do
        for k, v2 in pairs(luna.__callbacks[i]) do
            if v2 == sig then
                error("signal '" .. v2 .. "' already in use")
            end
        end
    end

    new_handler = {signal = sig, callback = handler}
    table.insert(luna.__callbacks, new_handler)

    return new_handler
end

function luna.delSignal(handler)
    for i, v in ipairs(luna.__callbacks) do
        if v == handler then
            return table.remove(luna.__callbacks, i)
        end
    end
end

--
-- Register script to Luna
--
function luna.registerScript(inf)
    if luna.__scriptinfo ~= nil then
        error 'script has already been registered'
    end

    if (type(inf.name) == 'string' and #inf.name ~= 0) and
       (type(inf.author) == 'string' and #inf.author ~= 0) and
       (type(inf.description) == 'string' and #inf.description ~= 0) and
       (type(inf.version) == 'string' and #inf.version ~= 0) then

       luna.__scriptinfo = inf
   else
       error "fields 'name', 'author', 'description' and 'version' are required"
   end
end

--
-- Some type goodness
--
function luna.types.channel:getName()
    return ({self:getChannelInfo()})[1]
end

function luna.types.channel:getCreationDate()
    return ({self:getChannelInfo()})[2]
end

function luna.types.channel:privmsg(msg)
    luna.privmsg(self:getName(), msg)
end



function luna.types.channel_user:getRegUser()
    n, u, h = self:getUserInfo()
    return luna.users.find{nick = n, user = u, host = h}
end

function luna.types.channel_user:respond(msg)
    nick = self:getUserInfo()

    self:getChannel():privmsg(string.format("%s: %s", nick, msg))
end

function luna.types.user:isOperator()
    return self:getFlags():find('o') ~= nil
end
