--
-- Luna API core functions
--

--
-- Utils
--
function table.copy(t)
    local t2 = {}
    for k,v in pairs(t) do
        t2[k] = v
    end
    return t2
end

-- Copy a metatables methods and introduce getter attributes
function make_getters(tab, attrs)
    local meth = table.copy(tab)

    tab.__index = function(self, key)
        if attrs[key] ~= nil then
            return meth[attrs[key]](self)
        else
            return meth[key]
        end
    end
end

-- Same with setters
function make_setters(tab, attrs)
    local meth = table.copy(tab)

    tab.__newindex = function(self, key, value)
        if attrs[key] ~= nil then
            meth[attrs[key]](self, value)
        else
            error(string.format('no such field \'%s\'', key))
        end
    end
end

-- And getters for library functions
function make_lib_getters(tab, attrs)
    tab = setmetatable(tab, {
        __index = function(self, key)
            if attrs[key] ~= nil then
                return tab[attrs[key]]()
            end
        end
    })
end

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

function luna.ctcp(target, ctcp, message)
    if message then
        luna.privmsg(target, string.format('\001%s %s\001', ctcp, message))
    else
        luna.privmsg(target, string.format('\001%s\001', ctcp))
    end
end

function luna.ctcpReply(target, ctcp, message)
    if message then
        luna.notice(target, string.format('\001%s %s\001', ctcp, message))
    else
        luna.notice(target, string.format('\001%s\001', ctcp))
    end
end

--
-- Add and delete signal handlers
--
function luna.addSignal(sig, handler)
    for i, v in ipairs(luna.__callbacks) do
        for k, v2 in pairs(luna.__callbacks[i]) do
            if v2 == sig then
                error("signal '" .. v2 .. "' already in use", 2)
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
-- While everything in luna.__scriptinfo can be changed at runtime,
-- all it really does is confuse the user. The scripts will respect
-- the value within __scriptinfo but the bot itself has its own
-- copy of these values.
--
-- TODO: Make bot only save lua_State and grab __scriptinfo
--
function luna.registerScript(inf)
    if luna.__scriptinfo ~= nil then
        error('script has already been registered', 2)
    end

    if (type(inf.name) == 'string' and #inf.name ~= 0) and
        (type(inf.author) == 'string' and #inf.author ~= 0) and
        (type(inf.description) == 'string' and #inf.description ~= 0) and
        (type(inf.version) == 'string' and #inf.version ~= 0) then

        luna.__scriptinfo = inf
    else
        error("fields 'name', 'author', 'description' " ..
        "and 'version' are required", 2)
    end
end

--
-- Some type goodness
--

---- Channel methods
--

function dump(t, max)
    local res = '{'

    if max == 0 then
        return 'overflow'
    end

    for key, val in pairs(t) do
        local str = ''

        if type(val) == 'string' then
            str = '"' .. val .. '"'
        elseif type(val) == 'number' then
            str = tostring(val)
        elseif type(val) == 'table' then
            i = i + 1
            str = dump(val, max - 1)
        elseif type(val) == 'function' then
            str = 'f()'
        else
            str = '<unknown@' .. type(val) .. '>'
        end

        if type(key) == 'number' then
            res = res .. string.format('%s, ', str)
        else
            res = res .. string.format('%s = %s, ', key, str)
        end
    end

    return res:sub(1, res:len() - 2) .. '}'
end

function luna.types.channel:getName()
    return ({self:getChannelInfo()})[1]
end

function luna.types.channel:getCreationDate()
    return ({self:getChannelInfo()})[2]
end

function luna.types.channel:privmsg(msg)
    luna.privmsg(self:getName(), msg)
end

make_getters(luna.types.channel, {
    name    = 'getName',
    created = 'getCreationDate',
    modes   = 'getModes',
    topic   = 'getTopic',
    userlist = 'getUserList'
})


---- Unknown users
--
function luna.types.source_user:__tostring()
    return string.format('%s!%s@%s', self:getUserInfo())
end

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

function luna.types.source_user:notice(msg)
    local nick = self:getUserInfo()
    luna.notice(nick, msg)
end

function luna.types.source_user:ctcp(ctcp, msg)
    local nick = self:getUserInfo()
    luna.ctcp(nick, ctcp, msg)
end

function luna.types.source_user:ctcpReply(ctcp, msg)
    local nick = self:getUserInfo()
    luna.ctcpReply(nick, ctcp, msg)
end

function luna.types.source_user:getNick()
    return ({self:getUserInfo()})[1]
end

function luna.types.source_user:getUser()
    return ({self:getUserInfo()})[2]
end

function luna.types.source_user:getHost()
    return ({self:getUserInfo()})[3]
end

make_getters(luna.types.source_user, {
    nick = 'getNick',
    user = 'getUser',
    host = 'getHost'
})

---- Known channel users
--
function luna.types.channel_user:getRegUser()
    return luna.users.find(self:getUserInfo())
end

function luna.types.channel_user:respond(msg)
    local nick = self:getUserInfo()
    self:getChannel():privmsg(string.format("%s: %s", nick, msg))
end

function luna.types.channel_user:isMe()
    return self.nick == ({luna.self.getUserInfo()})[1]
end

function luna.types.channel_user:getNick()
    return ({self:getUserInfo()})[1]
end

function luna.types.channel_user:getUser()
    return ({self:getUserInfo()})[2]
end

function luna.types.channel_user:getHost()
    return ({self:getUserInfo()})[3]
end

function luna.types.channel_user:notice(msg)
    local nick = self:getUserInfo()
    luna.notice(nick, msg)
end

function luna.types.channel_user:ctcp(ctcp, msg)
    local nick = self:getUserInfo()
    luna.ctcp(nick, ctcp, msg)
end

function luna.types.channel_user:ctcpReply(ctcp, msg)
    local nick = self:getUserInfo()
    luna.ctcpReply(nick, ctcp, msg)
end


make_getters(luna.types.channel_user, {
    status  = 'getStatus',
    modes   = 'getModes',
    channel = 'getChannel',
    info    = 'getUserInfo',
    nick    = 'getNick',
    user    = 'getUser',
    host    = 'getHost'
})

function luna.types.user:isOperator()
    return self:getFlags():find('o') ~= nil
end

make_getters(luna.types.user, {
    flags = 'getFlags',
    level = 'getLevel',
    id    = 'getId'
})

make_setters(luna.types.user, {
    flags = 'setFlags',
    level = 'setLevel',
    id    = 'setId'
})

---- Scripts
--
function luna.types.script:getFilename()
    return ({self:getScriptInfo()})[1]
end

function luna.types.script:getName()
    return ({self:getScriptInfo()})[2]
end

function luna.types.script:getDescription()
    return ({self:getScriptInfo()})[3]
end

function luna.types.script:getAuthor()
    return ({self:getScriptInfo()})[4]
end

function luna.types.script:getVersion()
    return ({self:getScriptInfo()})[5]
end

make_getters(luna.types.script, {
    file        = 'getFilename',
    name        = 'getName',
    description = 'getDescription',
    author      = 'getAuthor',
    version     = 'getVersion'
})

---- Some more utilities
--
function luna.self.getNick()
    return ({luna.self.getUserInfo()})[1]
end

function luna.self.getUser()
    return ({luna.self.getUserInfo()})[2]
end

function luna.self.getReal()
    return ({luna.self.getUserInfo()})[3]
end

function luna.self.getStarted()
    return ({luna.self.getRuntimeInfo()})[1]
end

function luna.self.getConnected()
    return ({luna.self.getRuntimeInfo()})[2]
end

make_lib_getters(luna.self, {
    nick = 'getNick',
    user = 'getUser',
    real = 'getReal',
    connected = 'getConnected',
    started   = 'getStarted'
})

make_lib_getters(luna.channels, {
    channellist = 'getChannelList'
})

make_lib_getters(luna.scripts, {
    scriptlist = 'getScriptList'
})

make_lib_getters(luna.users, {
    userlist = 'getUserList'
})


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

function string:colour(f,b)
    if not b then
        return string.format('\x03%02d%s\x03', f, self)
    else
        return string.format('\x03%02d,%02d%s\x03', f, b, self)
    end
end

function string:bold()
    return string.format('\x02%s\x02', self)
end

function string:underline()
    return string.format('\x1f%s\x1f', self)
end

function string:reverse()
    return string.format('\x16%s\x16', self)
end
