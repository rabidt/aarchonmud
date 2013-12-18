-- luareset
local reset_table =
{
    "interp",
    "commands",
    "arclib",
    "scripting",
    "utilities",
    "leaderboard"
}

local function luareset_usage( ch )
    sendtochar( ch,
[[
Syntax:
    luareset <file>

Valid args:
]])
    for _,arg in pairs(reset_table) do
        sendtochar( ch, arg.."\n\r")
    end
end

function do_luareset(ch, argument)
    local found=false
    for _,arg in pairs(reset_table) do
        if arg == argument then
            found=true
        end     
    end
    if not found then
        luareset_usage( ch )
        return
    end

    sendtochar(ch, "Loading "..argument..".lua\n\r")
    local f,err=loadfile( mud.luadir() .. argument..".lua")
    if not(f) then
        error(err)
    end
    sendtochar(ch, "Loaded.\n\r")
    sendtochar(ch, "Running "..argument..".lua\n\r")
    f()
    sendtochar(ch, argument..".lua reloaded successfully.\n\r")
end

-- scriptdump
local function scriptdumpusage( ch )
    sendtochar(ch, [[
scriptdump <userdir> <scriptname> [true|false]

Third argument (true/false) prints line numbers if true. Defaults to true
if not provided.
                   
Example: scriptdump vodur testscript false 
]])
end

function do_scriptdump( ch, argument )
    args=arguments(argument, true)
    if #args < 2 or #args > 3  then
        scriptdumpusage(ch)
        return
    end

    if not(args[3]=="false") then
        pagetochar( ch, linenumber(colorize(GetScript( args[1], args[2] ), ch)).."\n\r", true )
    else
        pagetochar( ch, colorize(GetScript( args[1], args[2] )).."\n\r", true )
    end

end

-- luaconfig

local syn_cfg_tbl=
{
    "keywords",
    "boolean",
    "nil",
    "function",
    "operator",
    "global",
    "comment",
    "string",
    "number"
}

local configs_table={}

local function show_luaconfig_usage( ch )
    pagetochar( ch,
[[

luaconfig list
luaconfig <name> <color char>

]])

end

local xtermcols=
{
    "r",
    "R",
    "g",
    "G",
    "y",
    "Y",
    "b",
    "B",
    "m",
    "M",
    "c",
    "C",
    "w",
    "W",
    "a",
    "A",
    "j",
    "J",
    "l",
    "L",
    "o",
    "O",
    "p",
    "P",
    "t",
    "T",
    "v",
    "V"
}

function do_luaconfig( ch, argument )
    local args=arguments(argument, true)

    if not(args[1]) then
        show_luaconfig_usage( ch )
        return
    end

    local cfg=configs_table[ch] -- maybe nil

    if (args[1] == "list") then
        sendtochar(ch,
                string.format("%-15s %s\n\r",
                    "Setting Name",
                    "Your setting") )

        for k,v in pairs(syn_cfg_tbl) do
            local char=cfg and cfg[v]

            sendtochar(ch,
                    string.format("%-15s %s\n\r",
                        v,
                        ( (char and ("\t"..char..char.."\tn")) or "") ) )
        end

        sendtochar( ch, "\n\rSupported colors:\n\r")
        for k,v in pairs(xtermcols) do
            sendtochar( ch, "\t"..v..v.." ")
        end
        sendtochar( ch, "\tn\n\r")
        return
    end

    for k,v in pairs(syn_cfg_tbl) do
        if (args[1]==v) then
            configs_table[ch]=configs_table[ch] or {}
            if not(args[2]) then
                configs_table[ch][v]=nil
                sendtochar( ch, "Config cleared for "..v.."\n\r")
                return
            end

            for l,w in pairs(xtermcols) do
                if w==args[2] then
                    configs_table[ch][v]=w
                    sendtochar( ch, "Config for "..v.." set to \t"..w..w.."\tn\n\r")
                    return
                end
            end
            
            sendtochar(ch, "Invalid argument: "..args[2].."\n\r")    
            return
        end
    end

    show_luaconfig_usage( ch )

end

function colorize( text, ch )
    config=(ch and configs_table[ch]) or {}
    local rtn={}
    local len=#text
    local i=0
    local word
    local waitfor
    local funtrack={}
    local nestlevel=0 -- how many functions are we inside

    while (i < len) do
        i=i+1
        local char=text:sub(i,i)

        if waitfor then
            if waitfor=='\n' 
                and waitfor==char 
                then
                waitfor=nil
                table.insert(rtn,"\tn"..char)
            elseif waitfor==']]' 
                and waitfor==text:sub(i,i+1) 
                then
                table.insert(rtn,"]]\tn")
                waitfor=nil
                i=i+1
            elseif waitfor=='--]]'
                and waitfor==text:sub(i,i+3)
                then
                table.insert(rtn,"--]]\tn")
                waitfor=nil
                i=i+3
            elseif char==waitfor then
                -- ends up handling ' and "
                waitfor=nil
                table.insert(rtn, char.."\tn")
            else
                -- waitfor didn't match, just push the char
                table.insert(rtn, char)
            end
        -- Literal strings
        elseif char=='"' or char=="'" then
            table.insert(rtn, "\t"..(config["string"] or 'r')..char)
            waitfor=char
        -- Multiline strings
        elseif char=='[' and text:sub(i+1,i+1) == '[' then
            table.insert(rtn, "\t"..(config["string"] or 'r').."[[")
            i=i+1
            waitfor=']]'
        -- Multiline comments
        elseif char=='-' and text:sub(i+1,i+3) == "-[[" then
            table.insert(rtn, "\t"..(config["comment"] or 'c').."--[[")
            i=i+3
            waitfor='--]]'
        -- Single line comments
        elseif char=='-' and text:sub(i+1,i+1) == '-' then
            table.insert(rtn, "\t"..(config["comment"] or 'c').."--")
            i=i+1
            waitfor='\n'
        elseif char=='\t' then
            table.insert(rtn, "    ")
        -- Operators
        elseif char=='[' or char==']'
            or char=='(' or char==')'
            or char=='=' or char=='%'
            or char=='<' or char=='>'
            or char=='{' or char=='}'
            or char=='/' or char=='*'
            or char=='+' or char=='-'
            or char==',' or char=='.'
            or char==":" or char==";"
            then
            table.insert(rtn, "\t"..(config["operator"] or 'G')..char.."\tn")
        -- Words
        elseif string.find(char, "%a") then
            local start,finish,word=string.find(text,"(%a[%w_%.]*)",i)
            i=finish
            if word=="function" then
                table.insert(funtrack,1,nestlevel)
                nestlevel=nestlevel+1
                table.insert(rtn, "\t"..(config["function"] or 'C')..word.."\tn")
            -- these two words account for do, while, if, and for
            elseif word=="do" or word=="if" then
                nestlevel=nestlevel+1
                table.insert(rtn, "\t"..(config["keywords"] or 'Y')..word.."\tn")
            elseif word=="end" then
                nestlevel=nestlevel-1
                if funtrack[1] and funtrack[1]==nestlevel then
                    table.remove(funtrack,1)
                    table.insert(rtn, "\t"..(config["function"] or 'C')..word.."\tn")
                else
                    table.insert(rtn, "\t"..(config["keywords"] or 'Y')..word.."\tn")
                end
            -- boolean
            elseif word=="true" or word=="false" then
                table.insert(rtn, "\t"..(config["boolean"] or 'r')..word.."\tn")
            -- 'keywords'
            elseif word=="and" or word=="in" or word=="repeat"
                or word=="break" or word=="local" or word=="return"
                or word=="for" or word=="then" or word=="else"
                or word=="not" or word=="elseif" or word=="if"
                or word=="or" or word=="until" or word=="while"
                then
                table.insert(rtn, "\t"..(config["keywords"] or 'Y')..word.."\tn")
            -- nil
            elseif word=="nil" then
                table.insert(rtn, "\t"..(config["nil"] or 'r')..word.."\tn")
            else
                -- Search globals
                local found=false
                for k,v in pairs(main_lib_names) do
                    if word==v then
                        table.insert(rtn, "\t"..(config["global"] or 'C')..word.."\tn")
                        found=true
                        break
                    end
                end

                -- Nothing special, just shove it
                if not(found) then
                    table.insert(rtn,word)
                end
            end
        -- Numbers
        elseif string.find(char, "%d") then
            local start,finish=string.find(text,"([%d%.]+)",i)
            word=text:sub(start,finish)
            i=finish
            table.insert(rtn, "\t"..(config["number"] or 'm')..word.."\tn")
        else
            -- Whatever else
            table.insert(rtn,char)
        end
    end

    return table.concat(rtn)

end

function save_luaconfig( ch )
    if not(configs_table[ch]) then return nil end

    rtn=serialize.save("cfg",configs_table[ch])
    return rtn
end

function load_luaconfig( ch, text )
    configs_table[ch]=loadstring(text)()
end

function wizhelp( ch, argument, commands )
    local args=arguments(argument)
    if args[1]=="level" or args[1]=="name" then
        table.sort( commands, function(a,b) return a[args[1]]<b[args[1]] end )

        if args[1]=="level" and args[2] then
            local old=commands
            commands=nil
            commands={}
            for i,v in ipairs(old) do
                if v.level==tonumber(args[2]) then
                    table.insert(commands, v)
                end
            end
        end
    elseif args[1]=="find" then
        local old=commands
        commands=nil
        commands={}
        for i,v in ipairs(old) do
            if string.find( v.name, args[2] ) then
                table.insert(commands, v)
            end
        end
    end

    local columns={}
    local numcmds=#commands
    local numrows=math.ceil(numcmds/3)
    
    for i,v in pairs(commands) do
        local row
        row=i%numrows
        if row==0 then row=numrows end

        columns[row]=columns[row] or ""
        columns[row]=string.format("%s %4s %-10s (%3d) ",
                columns[row],
                i..".",
                v.name,
                v.level)

    end

    pagetochar( ch, table.concat(columns, "\n\r")..
[[ 

   wizhelp <name|level>   -- List commands sorted by name or level.
   wizhelp level [number] -- List commands at given level.
   wizhelp find [pattern] -- Find commands matching given pattern (lua pattern matching).
]] )

end