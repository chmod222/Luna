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

---- Channel methods
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

---- Unknown users
--
function luna.types.source_user:privmsg(msg)
    local nick = self:getUserInfo()
    luna.privmsg(nick, msg)
end

function luna.types.source_user:getRegUser()
    local n, u, h = self:getUserInfo()
    return luna.users.find{nick = n, user = u, host = h}
end

function luna.types.source_user:respond(msg)
    local nick = self:getUserInfo()
    self:privmsg(string.format('%s: %s', nick, msg))
end

---- Known channel users
--
function luna.types.channel_user:getRegUser()
    local n, u, h = self:getUserInfo()
    return luna.users.find{nick = n, user = u, host = h}
end

function luna.types.channel_user:respond(msg)
    local nick = self:getUserInfo()
    self:getChannel():privmsg(string.format("%s: %s", nick, msg))
end

function luna.types.user:isOperator()
    return self:getFlags():find('o') ~= nil
end

---- Some more utilities
--

-- str = 'a b c'
-- str:split(' ') = {'a', 'b', 'c'}
function string:split(sep)
    local parts = {}
    local l = 1

    -- While there is a seperator within the string
    while self:find(sep, l) do
        local sep_start, sep_end = self:find(sep, l)

        -- Unless the substring between the last seperators was empty, add
        -- it to the results
        if sep_start ~= l then
            -- Add the part between l (last seperator end or string start) and
            -- sep_start
            table.insert(parts, self:sub(l, sep_start - 1))
        end

        -- put l after the seperator end
        l = sep_end + 1
    end

    if self:len() >= l then
        table.insert(parts, self:sub(l))
    end

    return parts
end
