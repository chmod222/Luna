--
-- Luna API core functions
--
luna.__callbacks = {}

--
-- Command aliases
--
function luna.join_channel(where, pass)
    if not pass then
        return luna.sendline(string.format('JOIN %s', where))
    else
        return luna.sendline(string.format('JOIN %s :%s', where, pass))
    end
end

function luna.quit(why)
    return luna.sendline('QUIT :' .. why)
end

function luna.change_nick(new)
    return luna.sendline('NICK ' .. new)
end

--
-- Types
--

-- Somewhat abstract supertype for anything you can message, not to be
-- instantiated
luna.addressable = {
    privmsg = function(self, what)
        return luna.sendline(string.format('PRIVMSG %s :%s',
            self:get_addr(),
            what))
    end,

    notice = function(self, what)
        return luna.sendline(string.format('NOTICE %s :%s',
            self:get_addr(),
            what))
    end,

    ctcp = function(self, what, args)
        if args then
            return self:privmsg(string.format('\001%s %s\001', what, args))
        else
            return self:privmsg(string.format('\001%s\001', what))
        end
    end,

    ctcp_reply = function(self, what, args)
        if args then
            return self:notice(string.format('\001%s %s\001', what, args))
        else
            return self:notice(string.format('\001%s\001', what))
        end
    end,
}

-- Type for unknown users (not in a known channel)
luna.user_meta = {}
luna.user = setmetatable({
    new = function(self, a)
        return setmetatable({ addr = a }, luna.user_meta)
    end,

    get_nick = function(self)
        return self.addr:sub(0, self.addr:find('!') - 1)
    end,

    get_user = function(self)
        return self.addr:sub(self.addr:find('!') + 1, self.addr:find('@') - 1)
    end,

    get_host = function(self)
        return self.addr:sub(self.addr:find('@') + 1, #self.addr)
    end,

    get_reguser = function(self)
        local id = luna.regusers.match_user(self.addr)

        if id then
            return luna.regusers.get_user_info(id)
        else
            return nil
        end
    end,

    -- implement interface for addressable
    get_addr = function(self)
        return self:get_nick()
    end
}, {
    __index = luna.addressable
})

luna.user_meta = {
    __index = luna.user,
    __tostring = function(self)
        return self.addr
    end,

    __eq = function(a, b)
        return a.addr == b.addr
    end
}

-- Type for a (known, joined) channel
luna.channel_meta = {}
luna.channel = setmetatable({
    new = function(self, c)
        assert(c ~= nil)

        return setmetatable({ name = c }, luna.channel_meta)
    end,

    get_name = function(self)
        return self.name
    end,

    get_user_mode = function(self, addr)
        return luna.channels.get_channel_user_info(self.name, addr).modes
    end,

    -- implement interface for addressable
    get_addr = function(self)
        return self:get_name()
    end,

    get_users = function(self)
        local userlist = {}

        for i, addr in ipairs(luna.channels.get_channel_users(self.name)) do
            table.insert(userlist, luna.channel_user:new(self, addr))
        end

        return userlist
    end,

    get_modes = function(self)
        return luna.channels.get_channel_info(self.name).modes
    end,

    get_topic = function(self)
        local meta = luna.channels.get_channel_info(self.name)

        return {
            topic = meta.topic,
            set_by = meta.topic_set_by,
            set_at = meta.topic_set_at
        }
    end,

    get_created = function(self)
        return luna.channels.get_channel_info(self.name).created
    end
}, {
    __index = luna.addressable
})

luna.channel_meta = {
    __index = luna.channel,
    __tostring = function(self) return self.name end,
    __eq = function(a, b) return a.name == b.name end
}

-- Type for a known user, joined in a known channel
luna.channel_user_meta = {}
luna.channel_user = setmetatable({
    new = function(self, c, u)
        assert(c ~= nil)

        ua = nil

        -- not a full address? Look it up
        if not u:find('!') then
            for i, a in ipairs(luna.channels.get_channel_users(c.name)) do
                if u:sub(0, u:find('!') - 1) == a then
                    ua = a
                    break
                end
            end
        else
            ua = u
        end

        return setmetatable({ channel = c, addr = u }, luna.channel_user_meta)
    end,

    get_channel_modes = function(self)
        return self.channel:get_user_mode(self.addr)
    end,

    respond = function(self, what)
        return self.channel:privmsg(self:get_nick() .. ': ' .. what)
    end
}, {
    __index = luna.user
})

luna.channel_user_meta = {
    __index = luna.channel_user,

    __tostring = function(self)
        return self.addr
    end,

    __eq = luna.user_meta.__eq
}


--
-- Regusers
--
luna.regusers = {
    __userlist = {},

    load_userlist = function(file)
        __userlist = {}

        for line in io.lines(file) do
            local parts = line:split(':')

            if #parts >= 4 then
                luna.regusers.__userlist[parts[1]:lower()] = {
                    id = parts[1],
                    mask = parts[2],
                    flags = parts[3],
                    level = parts[4]
                }
            end
        end
    end,

    write_userlist = function(file)
        local f = io.open(file, 'w')

        for i, user in pairs(luna.regusers.__userlist) do
            f:write(string.format('%s:%s:%s:%s\n',
                user.id, user.mask, user.flags, user.level))
        end

        f:close()
    end,

    match_user = function(addr)
        for i, user in pairs(luna.regusers.__userlist) do
            if addr:find(user.mask) then
                return user.id
            end
        end

        return nil
    end,

    get_user_info = function(id)
        if luna.regusers.__userlist[id:lower()] then
            return luna.regusers.__userlist[id:lower()]
        else
            error(string.format('no such user %q', id), 2)
        end
    end,

    set_user_info = function(id, newinf)
        if luna.regusers.__userlist[id:lower()] then
            if newinf.id and newinf.mask and newinf.flags and newinf.level then
                luna.regusers.__userlist[id:lower()] = nil
                luna.regusers.__userlist[newinf.id:lower()] = newin

                luna.regusers.write_userlist('users.txt')
            else
                error(string.format('invalid user table, one or more '
                                 .. 'of %q, %q, %q or &q not set',
                    'id', 'mask', 'flags', 'level'), 2)
            end
        else
            error(string.format('no such user %q', id), 2)
        end
    end,

    add_user = function(id, newinf)
        if luna.regusers.__userlist[id:lower()] then
            error(string.format('user %q already exists', id), 2)
        else
            if newinf.id and newinf.mask and newinf.flags and newinf.level then
                luna.regusers.__userlist[newinf.id:lower()] = newinf
                luna.regusers.write_userlist('users.txt')
            else
                error(string.format('invalid user table, one or more '
                                 .. 'of %q, %q, %q or &q not set',
                    'id', 'mask', 'flags', 'level'), 2)
            end
        end
    end,

    delete_user = function(id)
        if luna.regusers.__userlist[id:lower()] then
            luna.regusers.__userlist[id:lower()] = nil

            luna.regusers.write_userlist('users.txt')
        else
            error(string.format('no such user %q', id), 2)
        end
    end
}

-- Register script to Luna
--
-- Must be overridden by scripts
--
function luna.register_script()
    error('luna.register_script() not defined.')
end

-- Add a "managed" signal handler (automatically replace plaintext
-- values supplied from C with actual types and meta info and check
-- for a valid signal)
function luna.add_signal_handler(sig, id, fn)
    local substitutes = {
        idle = fn,
        connect = fn,
        disconnect = fn,
        raw = fn,

        private_message = function(who, where, what)
            who = luna.user:new(who)

            fn(who, what)
        end,

        public_message = function(who, where, what)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)

            fn(who, where, what)
        end,

        private_ctcp = function(who, where, what, args)
            who = luna.user:new(who)

            fn(who, what, args)
        end,

        private_ctcp_response = function(who, where, what, args)
            who = luna.user:new(who)

            fn(who, what, args)
        end,

        public_ctcp = function(who, where, what, args)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)

            fn(who, where, what, args)
        end,

        public_ctcp_response = function(who, where, what, args)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)

            fn(who, where, what, args)
        end,

        private_action = function(who, where, what)
            who = luna.user:new(who)

            fn(who, what)
        end,

        public_action = function(who, where, what)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)

            fn(who, where, what)
        end,

        private_command = function(who, where, what, args)
            who = luna.user:new(who)

            fn(who, what, args)
        end,

        public_command = function(who, where, what, args)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)

            fn(who, where, what, args)
        end,

        ping = fn,

        channel_join = function(who, where)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)

            fn(who, where)
        end,

        channel_join_sync = function(where)
            where = luna.channel:new(where)

            fn(where)
        end,

        channel_part = function(who, where, why)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)

            fn(who, where, why)
        end,

        user_quit = function(who, why)
            who = luna.user:new(who)

            fn(who, why)
        end,

        public_notice = function(who, where, what)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)

            fn(who, where, what)
        end,

        private_notice = function(who, where, what)
            who = luna.user:new(who)

            fn(who, what)
        end,

        nick_change = function(who_old, who_new)
            local addr = who_old:sub(who_old:find('!'), #who_old)

            who_new = luna.user:new(who_new .. addr)

            fn(who_old, who_new)
        end,

        invite = fn,

        topic_change = function(who, where, what)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)

            fn(who, where, what)
        end,

        user_kicked = function(who, where, whom, why)
            where = luna.channel:new(where)
            who = luna.channel_user:new(where, who)
            whom = luna.channel_user:new(where, whom)

            fn(who, where, whom, why)
        end,

        script_unload = function(filename)
            fn(luna.scripts.get_script_info(filename))
        end,

        script_load = function(filename)
            fn(luna.scripts.get_script_info(filename))
        end,
    }

    if substitutes[sig] then
        luna.add_raw_signal_handler(sig, id, substitutes[sig])
    else
        error(string.format('unknown signal %q', sig), 2)
    end
end

-- Add "unmanaged" signal handler. Handler functions receive the
-- parameters untouched.
function luna.add_raw_signal_handler(sig, id, fn)
    local newhandler = {
        signal = sig,
        callback = fn,
        enabled = true,
        priority = 0
    }

    luna.__callbacks[id] = newhandler
end

-- Helper function. Get handler or error out.
function luna.__get_signal_handler(id, fn)
    if luna.__callbacks[id] then
        fn(luna.__callbacks[id])
    else
        error(string.format('no signal handler with id "%s"', id), 3)
    end
end

-- Emit a signal, passing an arbitrary amount of parameters to it. Called via C.
function luna.emit_signal(signal, ...)
    local handlers = {}

    for id, handler in pairs(luna.__callbacks) do
        table.insert(handlers, handler)
    end

    table.sort(handlers, function(a, b)
        return a.priority < b.priority
    end)

    for i, handler in ipairs(handlers) do
        if handler.signal == signal and handler.enabled then
            handler.callback(unpack{...})
        end
    end
end

-- Delete a signal handler
function luna.delete_signal_handler(id)
    if luna.__callbacks[id] then
        -- disable because the list is cached during iteration
        luna.__callbacks[id].enabled = false
        luna.__callbacks[id] = nil
    else
        error(string.format('no signal handler with id "%s"', id), 2)
    end
end

-- Change signal handler priority
function luna.set_signal_handler_priority(id, priority)
    luna.__get_signal_handler(id, function(h)
        h.priority = priority or 0
    end)
end

-- Disable (state = true) or enable (state = false) a  signal handler.
function luna.disable_signal_handler(id, state)
    luna.__get_signal_handler(id, function(h)
        h.enabled = not state or false
    end)
end

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


function dump(t, max)
    local res = '{'

    if max == 0 then
        return 'overflow'
    end

    if not t then
        return 'nil'
    end

    for key, val in pairs(t) do
        local str = ''

        if type(val) == 'string' then
            str = '"' .. val .. '"'
        elseif type(val) == 'number' then
            str = tostring(val)
        elseif type(val) == 'table' then
            str = dump(val, max - 1)
        elseif type(val) == 'function' then
            str = 'f()'
        elseif type(val) == 'boolean' then
            str = tostring(val)
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


--
-- Initialization
--
luna.regusers.load_userlist('users.txt')
