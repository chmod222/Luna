function luna.register_script()
    return unpack{
        'Bootsrap',
        'Provides basic functionality and availability to load other scripts',
        'FliPPeh',
        '2.0'
    }
end

luna.add_signal_handler('connect', 1, function()
    luna.sendline('JOIN #luna')
end)

luna.add_signal_handler('public_command', 2, function(who, where, what, args)
    if who:get_reguser() then
        local user = who:get_reguser()

        if user.flags:find('o') then
            if what == 'load' then
                local ok, res = pcall(function()
                    return luna.scripts.load_script(args)
                end)

                if ok then
                    who:respond(string.format('Loaded script %q!', args))
                else
                    print(res)
                    who:respond(string.format('Failed to load script %q :(',
                        args))
                end
            elseif what == 'unload' then
                local ok, res = pcall(function()
                    return luna.scripts.unload_script(args)
                end)

                if ok then
                    who:respond(string.format('Unloaded script %q!', args))
                else
                    print(res)
                    who:respond(string.format('Failed to unload script %q :(',
                        args))
                end

            elseif what == 'reload' then
                local ok, res = pcall(function()
                    luna.scripts.unload_script(args)
                    return luna.scripts.load_script(args)
                end)

                if ok then
                    who:respond(string.format('Reloaded script %q!', args))
                else
                    print(res)
                    who:respond(string.format('Failed to reload script %q :(',
                        args))
                end

            end
        end
    end
end)
