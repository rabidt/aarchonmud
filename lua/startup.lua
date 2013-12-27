package.path = mud.luadir() .. "?.lua"

glob_tprintstr=require "tprint"
require "serialize"
glob_util=require "utilities"
require "leaderboard"

udtbl={} -- used to store game object tables, (read only proxies to origtbl)
envtbl={} -- game object script environments
origtbl={} -- where the REAL ud tables live
origenv={} -- where the REAL env tables live
interptbl={} -- key is game object pointer, table of desc=desc pointer, name=char name
delaytbl={} -- used on the C side mostly


function UdCnt()
    local cnt=0
    for k,v in pairs(udtbl) do
        cnt=cnt+1
    end
    return cnt
end

function EnvCnt()
    local cnt=0
    for k,v in pairs(envtbl) do
        cnt=cnt+1
    end
    return cnt
end

function MakeUdProxy(ud)
    local proxy={}
    setmetatable(proxy, {
            __index = ud,
            __newindex = getmetatable(ud)["__newindex"], 
            __tostring= function() return tostring(ud) end,
            __metatable=0 -- any value here protects it
            }
    )
    return proxy
end

function RegisterUd(ud)
    if ud == nil then
        error("ud is nil")
        return
    end
    origtbl[ud.tableid]=ud
    udtbl[ud.tableid]=MakeUdProxy(origtbl[ud.tableid])
    return udtbl[ud.tableid]
end

function UnregisterUd(lightud)
    if udtbl[lightud] then
        -- cancel delayed functions linked to object
        --cancel( udtbl[lightud] )

        setmetatable(origtbl[lightud], nil)
        rawset(origtbl[lightud], "tableid", nil)
        origtbl[lightud]=nil
        udtbl[lightud]={}
        udtbl[lightud]=nil
    end

    if envtbl[lightud] then
        setmetatable(origenv[lightud], nil)
        rawset(origenv[lightud], "udid", nil)
        origenv[lightud]=nil
        envtbl[lightud]={}
        envtbl[lightud]=nil
    end

    interptbl[lightud]=nil

end

function UnregisterDesc(desc)
    for k,v in pairs(interptbl) do
        if v.desc==desc then
            interptbl[k]=nil
        end
    end
end

function glob_rand(pcnt)
    return ( (mt.rand()*100) < pcnt)
end

function glob_randnum(low, high)
    return math.floor( (mt.rand()*(high+1-low) + low)) -- people usually want inclusive
end

function SaveTable( name, tbl, areaFname )
  if string.find(name, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end

  local dir=string.match(areaFname, "(%w+)\.are")
  if not os.rename(dir, dir) then
    os.execute("mkdir '" .. dir .. "'")
  end
  local f=io.open( dir .. "/" .. name .. ".lua", "w")
  out,saved=serialize.save(name,tbl)
  f:write(out)

  f:close()
end

function linenumber( text )
    local cnt=1
    local rtn={}
    table.insert(rtn, string.format("%3d. ", cnt))
    cnt=cnt+1

    for i=1,#text do
        local char=text:sub(i,i)
        table.insert(rtn, char)
        if char == '\n' then
            table.insert(rtn, string.format("%3d. ", cnt))
            cnt=cnt+1
        end
    end
            
    return table.concat(rtn)
end

function GetScript(subdir, name)
  if string.find(subdir, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end
  if string.find(name, "[^a-zA-Z0-9_/]") then
    error("Invalid character in name.")
  end


  local fname = mud.userdir() .. subdir .. "/" .. name .. ".lua"
  local f,err=io.open(fname,"r")
  if f==nil then
    error( fname .. "error: " ..  err)
  end

  rtn=f:read("*all")
  f:close()
  return rtn
end

function LoadTable(name, areaFname)
  if string.find(name, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end

  local dir=string.match(areaFname, "(%w+)\.are")
  local f=loadfile( dir .. "/"  .. name .. ".lua")
  if f==nil then
    return nil
  end

  return f()
end

-- Standard functionality avaiable for any env type
-- doesn't require access to env variables
function MakeLibProxy(tbl)
    local mt={
        __index=tbl,
        __newindex=function(t,k,v)
            error("Cannot alter library functions.")
        end,
        __metatable=0 -- any value here protects it
    }
    
    local proxy={}
    setmetatable(proxy, mt)
    return proxy
end

main_lib={  require=require,
		assert=assert,
		error=error,
		ipairs=ipairs,
		next=next,
		pairs=pairs,
		--pcall=pcall, -- remove so can't bypass infinite loop check
        print=print,
        select=select,
		tonumber=tonumber,
		tostring=tostring,
		type=type,
		unpack=unpack,
		_VERSION=_VERSION,
		--xpcall=xpcall, -- remove so can't bypass infinite loop check
		coroutine={create=coroutine.create,
					resume=coroutine.resume,
					running=coroutine.running,
					status=coroutine.status,
					wrap=coroutine.wrap,
					yield=coroutine.yield},
		string= {byte=string.byte,
				char=string.char,
				find=string.find,
				format=string.format,
				gmatch=string.gmatch,
				gsub=string.gsub,
				len=string.len,
				lower=string.lower,
				match=string.match,
				rep=string.rep,
				reverse=string.reverse,
				sub=string.sub,
				upper=string.upper},
				
		table={insert=table.insert,
				maxn=table.maxn,
				remove=table.remove,
				sort=table.sort,
				getn=table.getn,
				concat=table.concat},
				
		math={abs=math.abs,
				acos=math.acos,
				asin=math.asin,
				atan=math.atan,
				atan2=math.atan2,
				ceil=math.ceil,
				cos=math.cos,
				cosh=math.cosh,
				deg=math.deg,
				exp=math.exp,
				floor=math.floor,
				fmod=math.fmod,
				frexp=math.frexp,
				huge=math.huge,
				ldexp=math.ldexp,
				log=math.log,
				log10=math.log10,
				max=math.max,
				min=math.min,
				modf=math.modf,
				pi=math.pi,
				pow=math.pow,
				rad=math.rad,
				random=math.random,
				sin=math.sin,
				sinh=math.sinh,
				sqrt=math.sqrt,
				tan=math.tan,
				tanh=math.tanh},
		os={time=os.time,
			clock=os.clock,
			difftime=os.difftime},
        -- this is safe because we protected the game object and main lib
        --  metatables.
		setmetatable=setmetatable,
}

-- add script_globs to main_lib
for k,v in pairs(script_globs) do
    print("Adding "..k.." to main_lib.")
    if type(v)=="table" then
        for j,w in pairs(v) do
            print(j)
        end
    end
    main_lib[k]=v
end

-- Need to protect our library funcs from evil scripters
function ProtectLib(lib)
    for k,v in pairs(lib) do
        if type(v) == "table" then
            ProtectLib(v) -- recursion in case we add some nested libs
            lib[k]=MakeLibProxy(v)
        end
    end
    return MakeLibProxy(lib)
end

-- Before we protect it, we want to make a lit of names for syntax highligting
main_lib_names={}
for k,v in pairs(main_lib) do
    if type(v) == "function" then
        table.insert(main_lib_names, k)
    elseif type(v) == "table" then
        for l,w in pairs(v) do
            table.insert(main_lib_names, k.."."..l)
        end
    end
end
main_lib=ProtectLib(main_lib)


-- First look for main_lib funcs, then mob/area/obj funcs
CH_env_meta={
    __index=function(tbl,key)
        if main_lib[key] then
            return main_lib[key]
        elseif type(tbl.mob[key])=="function" then 
            return function(...) 
                        table.insert(arg, 1, tbl.mob)
                        return tbl.mob[key](unpack(arg)) 
                   end
        end
    end
}

OBJ_env_meta={
    __index=function(tbl,key)
        if main_lib[key] then
            return main_lib[key]
        elseif type(tbl.obj[key])=="function" then
            return function(...) 
                        table.insert(arg, 1, tbl.obj)
                        return tbl.obj[key](unpack(arg)) 
                   end
        end
    end
}

AREA_env_meta={
    __index=function(tbl,key)
        if main_lib[key] then
            return main_lib[key]
        elseif type(tbl.area[key])=="function" then
            return function(...)
                        table.insert(arg, 1, tbl.area) 
                        return tbl.area[key](unpack(arg)) 
                   end
        end
    end
}

ROOM_env_meta={
    __index=function(tbl,key)
        if main_lib[key] then
            return main_lib[key]
        elseif type(tbl.room[key])=="function" then
            return function(...)
                        table.insert(arg, 1, tbl.room)
                        return tbl.room[key](unpack(arg))
                   end
        end
    end
}

function MakeEnvProxy(env)
    local proxy={}
    proxy._G=proxy
    setmetatable(proxy, {
            __index = env,
            __newindex = function (t,k,v)
                if k=="tableid" then
                    error("Cannot alter tableid of environment.")
                elseif k=="udid" then
                    error("Cannot alter udid of environment.")
                else
                    rawset(t,k,v)
                end
            end,
            __metatable=0 -- any value here protects it
            }
    )

    return proxy
end

function new_script_env(ud, objname, meta)
    local env={ udid=ud.tableid, [objname]=ud}
    setmetatable(env, meta)
    origenv[ud.tableid]=env
    return MakeEnvProxy(env)
end

function mob_program_setup(ud, f)
    if envtbl[ud.tableid]==nil then
        envtbl[ud.tableid]=new_script_env(ud, "mob", CH_env_meta) 
    end
    setfenv(f, envtbl[ud.tableid])
    return f
end

function obj_program_setup(ud, f)
    if envtbl[ud.tableid]==nil then
        envtbl[ud.tableid]=new_script_env(ud, "obj", OBJ_env_meta)
    end
    setfenv(f, envtbl[ud.tableid])
    return f
end

function area_program_setup(ud, f)
    if envtbl[ud.tableid]==nil then
        envtbl[ud.tableid]=new_script_env(ud, "area", AREA_env_meta)
    end
    setfenv(f, envtbl[ud.tableid])
    return f
end

function room_program_setup(ud, f)
    if envtbl[ud.tableid]==nil then
        envtbl[ud.tableid]=new_script_env(ud, "room", ROOM_env_meta)
    end
    setfenv(f, envtbl[ud.tableid])
    return f
end

function interp_setup( ud, typ, desc, name)
    if interptbl[ud.tableid] then
        return 0, interptbl[ud.tableid].name
    end

    if envtbl[ud.tableid]== nil then
        if typ=="mob" then
            envtbl[ud.tableid]=new_script_env(ud,"mob", CH_env_meta)
        elseif typ=="obj" then
            envtbl[ud.tableid]=new_script_env(ud,"obj", OBJ_env_meta)
        elseif typ=="area" then
            envtbl[ud.tableid]=new_script_env(ud,"area", AREA_env_meta)
        elseif typ=="room" then
            envtbl[ud.tableid]=new_script_env(ud,"room", ROOM_env_meta)
        else
            error("Invalid type in interp_setup: "..typ)
        end
    end

    interptbl[ud.tableid]={name=name, desc=desc}
    return 1,nil
end

function run_lua_interpret(env, str )
    local f,err
    interptbl[env.udid].incmpl=interptbl[env.udid].incmpl or {}

    table.insert(interptbl[env.udid].incmpl, str)
    f,err=loadstring(table.concat(interptbl[env.udid].incmpl, "\n"))

    if not(f) then
        -- Check if incomplete, same way the real cli checks
        local ss,sf=string.find(err, "<eof>")
        if sf==err:len()-1 then
            return 1 -- incomplete
        else
           interptbl[env.udid].incmpl=nil
           error(err)
        end
    end

    interptbl[env.udid].incmpl=nil
    setfenv(f, env)
    f()
    return 0
end

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

function list_files ( path )
    local f=assert(io.popen('find ../player -type f -printf "%f\n"', 'r'))
    
    local txt=f:read("*all")
    f:close()
    
    local rtn={}
    for nm in string.gmatch( txt, "(%a+)\n") do
        table.insert(rtn, nm)
    end

    return rtn
end

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

local function luaquery_usage( ch )
    sendtochar( ch,
[[
luaquery <type> <selection> [filter] [sort] [width]

Types:
    area    - AREAs (area_list)
    op      - OBJPROTOs
    objs    - OBJs (object_list, live objects)
    mp      - MOBPROTOs
    mobs    - CHs (all mobs from char_list)
    room    - ROOMs
    mprog   - PROGs (all mprogs)

Selection:
    Determines which fields are shown on output. If '' or default then default
    values are used, otherwise fields supplied in a list separated by '|' 
    character.

Filter (optional):
    Expression used to filter which results are shown. Argument is a statement 
    that can be evaluated to a boolean result. 'x' can be used optionally to
    qualify referenced fields. It is necessary to use 'x' when invoking methods.

Sort (optional):
    One or more values determining the sort order of the output. Format is same
    as Selection.

Width (optional):
    An integer value which limits the width of the output columns to the given
    number of characters.

Notes: 
    'x' can be used optionally to qualify fields. 'x' is necessary to invoke
    methods (see examples).

    A field must be in the selection in order to be used in sort.

Examples:
    luaquery op level|vnum|shortdescr|x:extra("glow")|area.name 'otype=="weapon" and x:weaponflag("flaming")' level|x:extra("glow")

    Shows level, vnum, shortdescr, glow flag (true/false), and area.name for all
    OBJPROTOs that are weapons with flaming wflag. Sorted by level then by glow
    flag.

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
        default_sel="area.name|vnum|shortdescr"
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
    }
}

function do_luaquery( ch, argument)

    -- arg checking stuff
    args=arguments(argument, true)
    
    if not(args[1]) then
        luaquery_usage(ch)
        return
    end

    local typearg=args[1]
    local columnarg=args[2]
    local filterarg=args[3]
    local sortarg=args[4]
    local widtharg=args[5] and tonumber(args[5])

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
                    setmetatable({}, { __index=gobj } ) )
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
                    setmetatable({}, { __index=gobj,
                                       __newindex=function () 
                                            error("Can't set values with luaquery") 
                                            end
                                            } )
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
    table.insert(printing, 
            "|"..table.concat(hdrstr,"|").."|")

    for _,v in ipairs(output) do
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

  

    pagetochar(ch, table.concat(printing,"\n\r")..
            "\n\r\n\r"..
            "Total results: "..(#output).."\n\r")
    
end
