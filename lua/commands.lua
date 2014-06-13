-- luareset section
local reset_table =
{
--    "interp",
    "commands",
--    "arclib",
--    "scripting",
--    "utilities",
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
-- end luareset section

-- luaquery section
local query_results={}

local function luaquery_usage( ch )
    pagetochar( ch,
[[
{Cluaquery <type> <selection> [filter] [sort] [width] [limit]
    {xExecute a query and show first page of results.
{Cluaquery next
    {xShow the next page of results.

Types:
    area    - AREAs (area_list)
    op      - OBJPROTOs
    objs    - OBJs (object_list, live objects)
    mp      - MOBPROTOs
    mobs    - CHs (all mobs from char_list)
    room    - ROOMs
    reset   - RESETs (includes 'area' and 'room')
    help    - HELPs

    mprog   - PROGs (all mprogs, includes 'area')
    oprog   - PROGS (all oprogs, includes 'area')
    aprog   - PROGs (all aprogs, includes 'area')
    rprog   - PROGs (all rprogs, includes 'area')

    mtrig   - MTRIGs (all mprog triggers, includes 'area' and 'mobproto')
    otrig   - OTRIGs (all oprog triggers, includes 'area' and 'objproto')
    atrig   - ATRIGs (all aprog triggers, includes 'area')
    rtrig   - RTRIGs (all rprog triggers, includes 'area' and 'room')


Selection:
    Determines which fields are shown on output. If '' or default then default
    values are used, otherwise fields supplied in a list separated by '|' 
    character.

Filter (optional):
    Expression used to filter which results are shown. Argument is a statement 
    that can be evaluated to a boolean result. 'x' can be used optionally to
    qualify referenced fields. It is necessary to use 'x' when invoking methods.
    Using 'thisarea' in the filter produces results from current area only.

Sort (optional):
    One or more values determining the sort order of the output. Format is same
    as Selection.

Width (optional):
    An integer value which limits the width of the output columns to the given
    number of characters.

Limit (optional):
    An integer value which limits the number of results printed. If the value
    is positive then top results are printed. If the value is negative then
    bottom results are printed.

Notes: 
    'x' can be used optionally to qualify fields. 'x' is necessary to invoke
    methods (see examples).

    A field must be in the selection in order to be used in sort.

    For types listed above with "includes" in the description, these are variables
    included in the query (available for selection/filter/sort) that are not
    fields of the specified object type by default. For instance, PROG type does
    not have a field called 'area' but for the purposes of PROG queries it is
    accessible.

Examples:
    luaquery op level|vnum|shortdescr|x:extra("glow")|area.name 'otype=="weapon" and x:weaponflag("flaming")' level|x:extra("glow") 20

    Shows level, vnum, shortdescr, glow flag (true/false), and area.name for all
    OBJPROTOs that are weapons with flaming wflag. Sorted by level then by glow
    flag, with each column limited to 20 characters width.

    luaquery mtrig '' 'mobproto.level==90' '' 15
    Shows default selection for all mprog trigs that are attached to mobs of level 90.
    Results are unsorted but columns are limited to 15 characters.

    luaq reset '' 'command=="M" and arg1==10256'
    Show default selection for all resets of mob vnum 10256.

]])

end

local lqtbl={

    area={ 
        getfun=getarealist,
        default_sel="name"
    },

    op={
        getfun=function()
            local ops={}
            for _,area in pairs(getarealist()) do
                for _,op in pairs(area.objprotos) do
                    table.insert(ops, op)
                end
            end
            return ops
        end ,
        default_sel="vnum|level|shortdescr"
    },

    mp={
        getfun=function()
            local mps={}
            for _,area in pairs(getarealist()) do
                for _,mp in pairs(area.mobprotos) do
                    table.insert(mps, mp)
                end
            end
            return mps
        end,
        default_sel="vnum|level|shortdescr"
    },

    mobs={
        getfun=getmoblist,
        default_sel="vnum|level|shortdescr"
    },

    objs={
        getfun=getobjlist,
        default_sel="vnum|level|shortdescr"
    },

    room={
        getfun=function()
            local rooms={}
            for _,area in pairs(getarealist()) do
                for _,room in pairs(area.rooms) do
                    table.insert( rooms, room )
                end
            end
            return rooms
        end,
        default_sel="area.name|vnum|name"
    },


    reset={
        getfun=function()
            local resets={}
            for _,area in pairs(getarealist()) do
                for _,room in pairs(area.rooms) do
                    for _,reset in pairs(room.resets) do
                        table.insert( resets,
                                setmetatable( { ["area"]=area, ["room"]=room },
                                              {__index=reset} ) )
                    end
                end
            end
            return resets
        end,
        default_sel="area.name|room.name|room.vnum|command|arg1|arg2|arg3|arg4"
    },


    mprog={
        getfun=function()
            local progs={}
            for _,area in pairs(getarealist()) do
                for _,prog in pairs(area.mprogs) do
                    table.insert( progs,
                            setmetatable( { ["area"]=area}, {__index=prog}) )
                end
            end
            return progs
        end,
        default_sel="vnum"
    },

    oprog={
        getfun=function()
            local progs={}
            for _,area in pairs(getarealist()) do
                for _,prog in pairs(area.oprogs) do
                    table.insert( progs,
                            setmetatable( { ["area"]=area}, {__index=prog}) )
                end
            end
            return progs
        end,
        default_sel="vnum"
    },

    aprog={
        getfun=function()
            local progs={}
            for _,area in pairs(getarealist()) do
                for _,prog in pairs(area.aprogs) do
                    table.insert( progs,
                            setmetatable( { ["area"]=area}, {__index=prog}) )
                end
            end
            return progs
        end,
        default_sel="vnum"
    },

    rprog={
        getfun=function()
            local progs={}
            for _,area in pairs(getarealist()) do
                for _,prog in pairs(area.rprogs) do
                    table.insert( progs,
                            setmetatable( { ["area"]=area}, {__index=prog}) )
                end
            end
            return progs
        end,
        default_sel="vnum"
    },

    mtrig={
        getfun=function()
            local trigs={}
            for _,area in pairs(getarealist()) do
                for _,mp in pairs(area.mobprotos) do
                    for _,trig in pairs(mp.mtrigs) do
                        table.insert( trigs,
                                setmetatable( { ["area"]=area, ["mobproto"]=mp },
                                    {__index=trig}) )
                    end
                end
            end
            return trigs
        end,
        default_sel="mobproto.vnum|mobproto.shortdescr|trigtype|trigphrase|prog.vnum"
    },
    otrig={
        getfun=function()
            local trigs={}
            for _,area in pairs(getarealist()) do
                for _,op in pairs(area.objprotos) do
                    for _,trig in pairs(op.otrigs) do
                        table.insert( trigs,
                                setmetatable( { ["area"]=area, ["objproto"]=op },
                                    {__index=trig}) )
                    end
                end
            end
            return trigs
        end,
        default_sel="objproto.vnum|objproto.shortdescr|trigtype|trigphrase|prog.vnum"
    },
   
    atrig={
        getfun=function()
            local trigs={}
            for _,area in pairs(getarealist()) do
                for _,trig in pairs(area.atrigs) do
                    table.insert( trigs,
                            setmetatable( { ["area"]=area },
                                {__index=trig}) )
                end
            end
            return trigs
        end,
        default_sel="area.name|trigtype|trigphrase|prog.vnum"
    },
    
    rtrig={
        getfun=function()
            local trigs={}
            for _,area in pairs(getarealist()) do
                for _,room in pairs(area.rooms) do
                    for _,trig in pairs(room.rtrigs) do
                        table.insert( trigs,
                                setmetatable( { ["area"]=area, ["room"]=room },
                                    {__index=trig}) )
                    end
                end
            end
            return trigs
        end,
        default_sel="room.vnum|room.name|trigtype|trigphrase|prog.vnum"
    },

    help={
        getfun=gethelplist,
        default_sel="level|keywords"
    }


}

local function show_next_results( ch )
    if not(query_results[ch.name]) then return end

    local res=query_results[ch.name]

    local ind=res.index
    local scroll=(ch.scroll == 0 ) and 100 or ch.scroll
    local toind=math.min(ind+scroll-4, res.total ) -- -2 is normal, -3 for extra line at bottom, -4 for query at top

    local out={}
    table.insert(out, "Query: "..res.query)
    table.insert(out, res.header)
    for i=ind,toind do
        table.insert(out, res.results[i] )
    end

    table.insert( out, ("Results %d through %d (of %d)."):format( ind, toind, res.total) )

    if toind==res.total then
        query_results[ch.name]=nil
    else
        res.index=toind+1
    end

    pagetochar( ch, table.concat( out, "\n\r").."\n\r")
end

local luaq_nav=
{
    ["next"]=function( ch )
        show_next_results( ch )
    end
}

function do_luaquery( ch, argument)
    -- arg checking stuff
    args=arguments(argument, true)
    
    if not(args[1]) then
        luaquery_usage(ch)
        return
    end

    if luaq_nav[args[1]] then
        luaq_nav[args[1]]( ch )
        return
    end

    local typearg=args[1]
    local columnarg=args[2]
    local filterarg=args[3]
    local sortarg=args[4]
    local widtharg=args[5] and tonumber(args[5])
    local limitarg=args[6] and tonumber(args[6])

    -- what type are we searching ?
    local lqent=lqtbl[typearg]
    if lqent then
        getfun=lqent.getfun
    else
        sendtochar(ch,"Invalid type arg: "..typearg)
        return
    end

    -- which columns are we selecting for output ?
    if not(columnarg) then
        sendtochar( ch, "Must provide selection argument.\n\r")
        return
    elseif columnarg=="" or columnarg=="default" then
        columnarg=lqent.default_sel
    end

    local selection={}
    for word in columnarg:gmatch("[^|]+") do
        table.insert(selection, word)
    end

    -- let's get our result
    local lst=getfun()
    local rslt={}
    if filterarg then
        local filterfun=function(gobj)
            local vf,err=loadstring("return function(x) return "..filterarg.." end" )
            if err then error(err) return end
            setfenv(vf, 
                    setmetatable(
                        {
                            pairs=pairs
                        }, 
                        { 
                            __index=function(t,k)
                                if k=="thisarea" then
                                    return ch.room.area==gobj.area
                                else
                                    return gobj[k] 
                                end
                            end,
                            __newindex=function ()
                                error("Can't set values with luaquery")
                            end
                        } 
                    ) 
            )
            local val=vf()(gobj)
            if val then return true
            else return false end
        end

        for k,v in pairs(lst) do
            if filterfun(v) then table.insert(rslt, v) end
        end
    else
        rslt=lst
    end

    -- now populate output table based on our column selection
    local output={}
    for _,gobj in pairs(rslt) do
        local line={}
        for _,sel in ipairs(selection) do
            local vf,err=loadstring("return function(x) return "..sel.." end")
            if err then sendtochar(ch, err) return  end
            setfenv(vf,
                    setmetatable(
                        {
                            pairs=pairs
                        }, 
                        {   __index=gobj,
                            __newindex=function () 
                                error("Can't set values with luaquery") 
                            end
                        } 
                    )
            )
            table.insert(line, { col=sel, val=tostring(vf()(gobj))} )
        end
        table.insert(output, line)

    end

    if #output<1 then
        sendtochar( ch, "No results.\n\r")
        return
    end

    -- now sort
    if sortarg and not(sortarg=="") then
        local sorts={}
        for srt in sortarg:gmatch("[^|]+") do
            table.insert(sorts, srt)
        end

        local fun
        fun=function(a,b,lvl)
            local aval
            for k,v in pairs(a) do
                if v.col==sorts[lvl] then
                    aval=v.val
                    break
                end
            end
            
            if not(aval) then
                error("Bad sort argument '"..sorts[lvl].."'\n\r")
            end

            local bval
            for k,v in pairs(b) do
                if v.col==sorts[lvl] then
                    bval=v.val
                    break
                end
            end

            if tonumber(aval) then aval=tonumber(aval) end
            if tonumber(bval) then bval=tonumber(bval) end
            if aval==bval and sorts[lvl+1] then
                return fun(a, b, lvl+1)
            else
                return aval>bval
            end
        end

        local status,err=pcall( function()
            table.sort(output, function(a,b) return fun(a,b,1) end )
        end)
        if not(status) then
            sendtochar(ch,err.."\n\r")
            return
        end

    end


    -- NOW PRINT
     -- first scan through to determine column widths
    local widths={}
    local hdr={}
    for _,v in pairs(output) do
        for _,v2 in ipairs(v) do
            if not(widths[v2.col]) then
                widths[v2.col]=widtharg and 
                    math.min(util.strlen_color(v2.val), widtharg) or
                    util.strlen_color(v2.val)
                table.insert(hdr, v2.col)
            else
                local ln=widtharg and
                    math.min(util.strlen_color(v2.val), widtharg) or
                    util.strlen_color(v2.val)
                if ln>widths[v2.col] then
                    widths[v2.col]=ln
                end
            end
        end
    end

    local printing={}
    -- header
    local hdrstr={}
    for _,v in ipairs(hdr) do
        table.insert(hdrstr,
                util.format_color_string( v, widths[v]+1))
    end
    local hdr="|"..table.concat(hdrstr,"|").."|"

    local iter
    if not(limitarg) then
        iter=ipairs
    elseif limitarg<0 then
        iter=function(tbl)
            local i=math.max(1,#tbl+limitarg) -- limitarg is negative
            local limit=#tbl
            return function()
                i=i+1
                if i<=limit then return i,tbl[i] end
            end
        end
    else
        iter=function(tbl)
            local i=0
            local limit=math.min(#tbl, limitarg)
            return function()
                i=i+1
                if i<=limit then return i,tbl[i] end
            end
        end
    end

    for _,v in iter(output) do
        local line={}
        for _,v2 in ipairs(v) do
            local cc=v2.val:len() - util.strlen_color(v2.val)
            local width=widths[v2.col]
            table.insert( line,
                    util.format_color_string(
                        v2.val,
                        widths[v2.col]
                    )
                    .." {x"
            )
        end
        local ln=table.concat(line,"|")
        table.insert(printing, 
                "|"..ln.."|")
    end

    -- stick in in query_results table for browsing
    query_results[ch.name]=
    {
        query=argument,
        index=1,
        total=#printing,
        header=hdr,
        results=printing
    }

    show_next_results( ch )

end
-- end luaquery section

-- luaconfig section
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

configs_table = configs_table or {}

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

-- end luaconfig section

-- scriptdump section
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
-- end scriptdump section

-- wizhelp section
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
--end wizhelp section

-- alist section
local alist_col={
    "vnum",
    "name",
    "builders",
    "minvnum",
    "maxvnum",
    "security"
}

local function alist_usage( ch )
    sendtochar( ch,
[[
Syntax:  alist
         alist [column name] <ingame>
         alist find [text]


'alist' with no argument shows all areas, sorted by area vnum.

With a column name argument, the list is sorted by the given column name.
'ingame' is used as an optional 3rd argument to show only in game areas.

With 'find' argument, the area names and builders are searched for the
given text (case sensitive)  and only areas that match are shown.

Columns:
]])

    for k,v in pairs(alist_col) do
        sendtochar( ch, v.."\n\r")
    end
end

function do_alist( ch, argument )
    local args=arguments(argument, true)
    local sort
    local filterfun

    if not(args[1]) then
        sort="vnum"
    elseif args[1]=="find" then
        sort="vnum"
        if not(args[2]) then
            alist_usage( ch )
            return
        end
        filterfun=function(area)
            if area.name:find(args[2])
                or area.builders:find(args[2])
                then
                return true
            end
            return false
        end
    else
        for k,v in pairs(alist_col) do
            if v==args[1] then
                sort=v
                break
            end
        end
        if args[2]=="ingame" then
            filterfun=function(area)
                return area.ingame
            end
        end
    end

    if not(sort) then
        alist_usage( ch )
        return
    end

    local data={}
    for _,area in pairs(getarealist()) do
        local fil
        if filterfun then
            fil=filterfun(area)
        else
            fil=true
        end

        if fil then
            local row={}
            for _,col in pairs(alist_col) do
                row[col]=area[col]
            end
            table.insert(data, row)
        end
    end

    table.sort( data, function(a,b) return a[sort]<b[sort] end )

    local output={}
    --header
    table.insert( output,
            string.format("[%3s] [%-26s] [%-20s] (%-6s-%6s) [%-3s]",
                "Num", "Name", "Builders", "Lvnum", "Mvnum", "Sec"))
    for _,v in ipairs(data) do
        table.insert( output,
                string.format("[%3d] [%-26s] [%-20s] (%-6d-%6d) [%-3d]",
                    v.vnum,
                    util.format_color_string(v.name,25).." {x",
                    v.builders,
                    v.minvnum,
                    v.maxvnum,
                    v.security)
        )
    end

    pagetochar( ch, table.concat(output, "\n\r").."\n\r" )

end
--end alist section

--mudconfig section
local function mudconfig_usage(ch)
    sendtochar(ch,
[[
Syntax:
   mudconfig                  -- get the current value for all options
   mudconfig <option>         -- get the current value for given option
   mudconfig <option> <value> -- set a new value for the given option
]])
end
function do_mudconfig( ch, argument )
    local reg=debug.getregistry()
    local cnt=0
    for k,v in pairs(reg) do
        if tostring(v)=="CH" then
            cnt=cnt+1
        end
    end

    sendtochar( ch, cnt.."\n\r")
    sendtochar( ch, #getcharlist().."\n\r")

    local args=arguments(argument)
    local nargs=#args
    -- setting
    if nargs==2 then
        -- lua won't parse/convert booleans for us
        if args[2]=="true" or args[2]=="TRUE" then
            args[2]=true
        elseif args[2]=="false" or args[2]=="FALSE" then
            args[2]=false
        end

        mudconfig(unpack(args))
        -- show result
        do_mudconfig( ch, args[1])
        sendtochar(ch, "Done.\n\r")
        return
    end

    if nargs==1 then
        local rtn=mudconfig(args[1])
        sendtochar(ch, "%-20s %s\n\r", args[1], tostring(rtn))
        return
    end

    if nargs==0 then
        local sorted={}
        for k,v in pairs(mudconfig()) do
            table.insert(sorted, { key=k, value=v } )
        end

        table.sort( sorted, function(a,b) return a.key<b.key end )

        for _,v in ipairs(sorted) do
            sendtochar(ch, "%-20s %s\n\r", v.key, tostring(v.value))
        end
        return
    end

    mudconfig_usage(ch)
    return
end
--end mudconfig section

-- perfmon section
local pulsetbl={}
local pulseind=1
local sectbl={}
local secind=1
local mintbl={}
local minind=1
local hourtbl={}
local hourind=1
function log_perf( val )
    pulsetbl[pulseind]=val

    if pulseind<4 then 
        pulseind=pulseind+1
        return 
    else
        pulseind=1
    end

    -- 1 second complete, add to table
    local total=0
    for i=1,4 do
        total=total+pulsetbl[i]
    end

    sectbl[secind]=total/4

    if secind<60 then
        secind=secind+1
        return
    else
        secind=1
    end

    -- 1 minute complete, add to table
    total=0
    for i=1,60 do
        total=total+sectbl[i]
    end

    mintbl[minind]=total/60

    if minind<60 then
        minind=minind+1
        return
    else
        minind=1
    end

    -- 1 hour complete, add to table
    total=0
    for i=1,60 do
        total=total+mintbl[i]
    end

    hourtbl[hourind]=total/60

    if hourind<24 then
        hourind=hourind+1
        return
    else
        hourind=1
    end
        
end

function do_perfmon( ch, argument )
    local function avg( tbl )
        local cnt=0
        local ttl=0
        for k,v in pairs(tbl) do
            cnt=cnt+1
            ttl=ttl+v
        end

        return ttl/cnt
    end

    pagetochar( ch,
([[
Averages
  %3d Pulses:   %f
  %3d Seconds:  %f
  %3d Minutes:  %f
  %3d Hours:    %f
]]):format( #pulsetbl, avg(pulsetbl),
            #sectbl, avg(sectbl),
            #mintbl, avg(mintbl),
            #hourtbl, avg(hourtbl) ) )

end

--end perfmon section
