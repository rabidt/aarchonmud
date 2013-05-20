package.path = mud.luadir() .. "?.lua"

--require "tprint"
require "serialize"
require "utilities"

--function send_nocr (...)
  --  say (table.concat {...})
--end -- send_nocr

--function send (...)
  --  say (table.concat {...})
--end

udtbl={} -- used to store tables with userdata, we clear it out at the end of every script


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


function cleanupold()
    if udtbl==nil then
        return
    end

    -- DESTROY
    for _,v in ipairs (udtbl) do
        setmetatable(v,nil)
        rawset(v,"tableid",nil) 
        v={}
        v=nil
    end
    udtbl={}
end

function rand(pcnt)
    return ( (mt.rand()*100) < pcnt)
end

function randnum(low, high)
    return math.floor( (mt.rand()*(high+1-low) + low)) -- people usually want inclusive
end

local lio=io
io=nil

function savetbl(subdir, name, tbl)
  if string.find(name, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end
 
  if string.find(subdir, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end

  local f=lio.open(mud.userdir() .. subdir .. "/" .. name .. ".lua", "w")
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
    getobjworld=getobjworld,
    mobhere=mobhere,
    objexists=objexists
}

CH_env_meta={
    __index=CH_env_lib
}

function new_CH_env()
    local o={}
    setmetatable(o, CH_env_meta)
    o={
    mob={},
    send=function(...)
        o.mob:say(table.concat{...})
    end,
    send_nocr=function(...)
        o.mob:say(table.concat{...})
    end,
    log=function(arg)
        o.mob:log(arg)
    end,
    say=function(arg)
        o.mob:say(arg)
    end,
    emote=function(arg)
        o.mob:emote(arg)
    end,
    mdo=function(arg)
        o.mob:mdo(arg)
    end,
    asound=function(arg)
        o.mob:asound(arg)
    end,
    gecho=function(arg)
        o.mob:gecho(arg)
    end,
    zecho=function(arg)
        o.mob:zecho(arg)
    end,
    kill=function(arg)
        o.mob:kill(arg)
    end,
    assist=function(arg)
        o.mob:assist(arg)
    end,
    junk=function(arg)
        o.mob:junk(arg)
    end,
    echo=function(arg)
        o.mob:echo(arg)
    end,
    echoaround=function(arg)
        o.mob:echoaround(arg)
    end,
    echoat=function(arg)
        o.mob:echoat(arg)
    end,
    mload=function(arg)
        o.mob:mload(arg)
    end,
    oload=function(arg)
        o.mob:oload(arg)
    end,
    purge=function(arg)
        o.mob:purge(arg)
    end,
    goto=function(arg)
        o.mob:goto(arg)
    end,
    at=function(arg)
        o.mob:at(arg)
    end,
    transfer=function(arg)
        o.mob:transfer(arg)
    end,
    gtransfer=function(arg)
        o.mob:gtransfer(arg)
    end,
    otransfer=function(arg)
        o.mob:otransfer(arg)
    end,
    force=function(arg)
        o.mob:force(arg)
    end,
    gforce=function(arg)
        o.mob:gforce(arg)
    end,
    vforce=function(arg)
        o.mob:vforce(arg)
    end,
    cast=function(arg)
        o.mob:cast(arg)
    end,
    damage=function(arg)
        o.mob:damage(arg)
    end,
    remember=function(arg)
        o.mob:remember(arg)
    end,
    forget=function(arg)
        o.mob:forget(arg)
    end,
    delay=function(arg)
        o.mob:delay(arg)
    end,
    cancel=function(arg)
        o.mob:cancel(arg)
    end,
    call=function(arg)
        o.mob:call(arg)
    end,
    flee=function(arg)
        o.mob:flee(arg)
    end,
    remove=function(arg)
        o.mob:remove(arg)
    end,
    remort=function(arg)
        o.mob:remort(arg)
    end,
    qset=function(arg)
        o.mob:qset(arg)
    end,
    qadvance=function(arg)
        o.mob:qadvance(arg)
    end,
    reward=function(arg)
        o.mob:reward(arg)
    end,
    peace=function(arg)
        o.mob:peace(arg)
    end,
    restore=function(arg)
        o.mob:restore(arg)
    end,
    setact=function(arg)
        o.mob:setact(arg)
    end,
    hit=function(arg)
        o.mob:hit(arg)
    end,
    randchar=function(arg)
        o.mob:randchar(arg)
    end}
    return o
end

function program_setup(ud, f)
    if ud.env==nil then
      rawset(ud, "env", new_CH_env())
      ud.env.mob=ud
    end
    setfenv(f, ud.env)
    return f
end

os.execute=nil
os.rename=nil
os.remove=nil
os.exit=nil
os.setlocale=nil
