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
    mobhere=mobhere
}

CH_env_meta={
    __index=CH_env_lib
}

function new_CH_env()
    local o={}
    setmetatable(o, CH_env_meta)
    setfenv(1,o)
    mob={}
    function send(...)
        mob:say(table.concat{...})
    end
    function send_nocr(...)
        mob:say(table.concat{...})
    end
    function log(arg)
        mob:log(arg)
    end
    function say(arg)
        mob:say(arg)
    end
    function emote(arg)
        mob:emote(arg)
    end
    function mdo(arg)
        mob:mdo(arg)
    end
    function asound(arg)
        mob:asound(arg)
    end
    function gecho(arg)
        mob:gecho(arg)
    end
    function zecho(arg)
        mob:zecho(arg)
    end
    function kill(arg)
        mob:kill(arg)
    end
    function assist(arg)
        mob:assist(arg)
    end
    function junk(arg)
        mob:junk(arg)
    end
    function echo(arg)
        mob:echo(arg)
    end
    function echoaround(arg)
        mob:echoaround(arg)
    end
    function echoat(arg)
        mob:echoat(arg)
    end
    function mload(arg)
        mob:mload(arg)
    end
    function oload(arg)
        mob:oload(arg)
    end
    function purge(arg)
        mob:purge(arg)
    end
    function goto(arg)
        mob:goto(arg)
    end
    function at(arg)
        mob:at(arg)
    end
    function transfer(arg)
        mob:transfer(arg)
    end
    function gtransfer(arg)
        mob:gtransfer(arg)
    end
    function otransfer(arg)
        mob:otransfer(arg)
    end
    function force(arg)
        mob:force(arg)
    end
    function gforce(arg)
        mob:gforce(arg)
    end
    function vforce(arg)
        mob:vforce(arg)
    end
    function cast(arg)
        mob:cast(arg)
    end
    function damage(arg)
        mob:damage(arg)
    end
    function remember(arg)
        mob:remember(arg)
    end
    function forget(arg)
        mob:forget(arg)
    end
    function delay(arg)
        mob:delay(arg)
    end
    function cancel(arg)
        mob:cancel(arg)
    end
    function call(arg)
        mob:call(arg)
    end
    function flee(arg)
        mob:flee(arg)
    end
    function remove(arg)
        mob:remove(arg)
    end
    function remort(arg)
        mob:remort(arg)
    end
    function qset(arg)
        mob:qset(arg)
    end
    function qadvance(arg)
        mob:qadvance(arg)
    end
    function reward(arg)
        mob:reward(arg)
    end
    function peace(arg)
        mob:peace(arg)
    end
    function restore(arg)
        mob:restore(arg)
    end
    function setact(arg)
        mob:setact(arg)
    end
    function hit(arg)
        mob:hit(arg)
    end
    function randchar(arg)
        mob:randchar(arg)
    end
    return o
end

os.execute=nil
os.rename=nil
os.remove=nil
os.exit=nil
os.setlocale=nil
