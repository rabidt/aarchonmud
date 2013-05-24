package.path = mud.luadir() .. "?.lua"

require "tprint"
require "serialize"
require "utilities"

--function send_nocr (...)
  --  say (table.concat {...})
--end -- send_nocr

--function send (...)
  --  say (table.concat {...})
--end

udtbl={} -- used to store tables with userdata, we clear it out at the end of every script
current_env={}

function RegisterUd(ud)
    if ud == nil then
        error("ud is nil")
        return
    end

    udtbl[ud.tableid]=ud
    return
end

function UnregisterUd(lightud)
    if not(udtbl[lightud]) then return end

    setmetatable(udtbl[lightud], nil)
    rawset(udtbl[lightud], "tableid",nil)
    udtbl[lightud]={}
    udtbl[lightud]=nil
end


function cleanup()
end

function rand(pcnt)
    return ( (mt.rand()*100) < pcnt)
end

function randnum(low, high)
    return math.floor( (mt.rand()*(high+1-low) + low)) -- people usually want inclusive
end

function savetbl(subdir, name, tbl)
  if string.find(name, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end
 
  if string.find(subdir, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end

  local f=io.open(mud.userdir() .. subdir .. "/" .. name .. ".lua", "w")
  out,saved=serialize.save(name,tbl)
  f:write(out)

  f:close()
end

function loadtbl(subdir, name)
  if string.find(subdir, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end

  if string.find(name, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end

  local f=loadfile(mud.userdir() .. subdir .. "/"  .. name .. ".lua")
  if f==nil then 
    return nil 
  end
  return f()
end

CH_env_lib={  require=require,
    assert=assert,
    error=error,
    ipairs=ipairs,
    next=next,
    pairs=pairs,
    pcall=pcall,
    print=print,
    select=select,
    tonumber=tonumber,
    tostring=tostring,
    type=type,
    unpack=unpack,
    _VERSION=_VERSION,
    xpcall=xpcall,
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

    -- okay now our stuff
    tprint=function(tbl)
        local str={}
        if current_env.mob then
            tprint(str, tbl)
            current_env.mob:say(table.concat(str))
        end
    end,
    rand=rand,
    randnum=randnum,
    getobjworld=getobjworld,
    mobhere=mobhere,
    objexists=objexists
}

OBJ_env_lib={
require=require,
    assert=assert,
    error=error,
    ipairs=ipairs,
    next=next,
    pairs=pairs,
    pcall=pcall,
    print=print,
    select=select,
    tonumber=tonumber,
    tostring=tostring,
    type=type,
    unpack=unpack,
    _VERSION=_VERSION,
    xpcall=xpcall,
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
        difftime=os.difftime}
}

CH_env_meta={
    __index=function(table,key)
        if table.mob[key] then 
            return function(...) table.mob[key](table.mob, unpack(arg)) end
        else 
            return CH_env_lib[key] 
        end
    end
    
}

OBJ_env_meta={
    __index=function(table,key)
        if table.obj[key] then
            return function(...) table.obj[key](table.obj, unpack(arg)) end
        else
            return OBJ_env_lib[key]
        end
    end
}

function new_OBJ_env()
    local o={}
    setmetatable(o, OBJ_env_meta)
    return o
end

function new_CH_env()
    local o={}
    setmetatable(o, CH_env_meta)
    return o
end

function mob_program_setup(ud, f)
    if ud.env==nil then
      rawset(ud, "env", new_CH_env())
      ud.env.mob=ud
    end
    current_env=ud.env
    setfenv(f, ud.env)
    return f
end

function obj_program_setup(ud, f)
    if ud.env==nil then
      rawset(ud, "env", new_OBJ_env())
      ud.env.obj=ud
    end
    current_env=ud.env
    setfenv(f, ud.env)
    return f
end

os.execute=nil
os.rename=nil
os.remove=nil
os.exit=nil
os.setlocale=nil
