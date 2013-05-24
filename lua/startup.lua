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
    o.mob={}
    o.send=function(...)
        o.mob:say(table.concat{...})
    end
    o.send_nocr=function(...)
        o.mob:say(table.concat{...})
    end
    o.log=function(arg)
        o.mob:log(arg)
    end
    o.say=function(arg)
        o.mob:say(arg)
    end
    o.emote=function(arg)
        o.mob:emote(arg)
    end
    o.mdo=function(arg)
        o.mob:mdo(arg)
    end
    o.asound=function(arg)
        o.mob:asound(arg)
    end
    o.gecho=function(arg)
        o.mob:gecho(arg)
    end
    o.zecho=function(arg)
        o.mob:zecho(arg)
    end
    o.kill=function(arg)
        o.mob:kill(arg)
    end
    o.assist=function(arg)
        o.mob:assist(arg)
    end
    o.junk=function(arg)
        o.mob:junk(arg)
    end
    o.echo=function(arg)
        o.mob:echo(arg)
    end
    o.echoaround=function(arg)
        o.mob:echoaround(arg)
    end
    o.echoat=function(arg)
        o.mob:echoat(arg)
    end
    o.mload=function(arg)
        o.mob:mload(arg)
    end
    o.oload=function(arg)
        o.mob:oload(arg)
    end
    o.purge=function(arg)
        o.mob:purge(arg)
    end
    o.goto=function(arg)
        o.mob:goto(arg)
    end
    o.at=function(arg)
        o.mob:at(arg)
    end
    o.transfer=function(arg)
        o.mob:transfer(arg)
    end
    o.gtransfer=function(arg)
        o.mob:gtransfer(arg)
    end
    o.otransfer=function(arg)
        o.mob:otransfer(arg)
    end
    o.force=function(arg)
        o.mob:force(arg)
    end
    o.gforce=function(arg)
        o.mob:gforce(arg)
    end
    o.vforce=function(arg)
        o.mob:vforce(arg)
    end
    o.cast=function(arg)
        o.mob:cast(arg)
    end
    o.damage=function(arg)
        o.mob:damage(arg)
    end
    o.remember=function(arg)
        o.mob:remember(arg)
    end
    o.forget=function(arg)
        o.mob:forget(arg)
    end
    o.delay=function(arg)
        o.mob:delay(arg)
    end
    o.cancel=function(arg)
        o.mob:cancel(arg)
    end
    o.call=function(arg)
        o.mob:call(arg)
    end
    o.flee=function(arg)
        o.mob:flee(arg)
    end
    o.remove=function(arg)
        o.mob:remove(arg)
    end
    o.remort=function(arg)
        o.mob:remort(arg)
    end
    o.qset=function(arg)
        o.mob:qset(arg)
    end
    o.qadvance=function(arg)
        o.mob:qadvance(arg)
    end
    o.reward=function(arg)
        o.mob:reward(arg)
    end
    o.peace=function(arg)
        o.mob:peace(arg)
    end
    o.restore=function(arg)
        o.mob:restore(arg)
    end
    o.setact=function(arg)
        o.mob:setact(arg)
    end
    o.hit=function(arg)
        o.mob:hit(arg)
    end
    o.randchar=function(arg)
        o.mob:randchar(arg)
    end
    return o
end

function program_setup(ud, f)
    if ud.env==nil then
      rawset(ud, "env", new_CH_env())
      ud.env.mob=ud
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
