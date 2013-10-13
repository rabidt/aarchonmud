package.path = mud.luadir() .. "?.lua"

tprintstr=require "tprint"
require "serialize"
require "utilities"
require "leaderboard"

udtbl={} -- used to store tables with userdata, we clear it out at the end of every script
envtbl={}

function MakeUdProxy(ud)
    local proxy={}
    setmetatable(proxy, {
            __index = ud,
            __newindex = function (t,k,v)
                error("Cannot set values on game objects.")
            end
            }
    )
    return proxy
end

function RegisterUd(ud)
    if ud == nil then
        error("ud is nil")
        return
    end

    udtbl[ud.tableid]=ud
    return udtbl[ud.tableid]
end

function UnregisterUd(lightud)
    if not(udtbl[lightud]) then return end

    setmetatable(udtbl[lightud], nil)
    rawset(udtbl[lightud], "tableid",nil)
    udtbl[lightud]={}
    udtbl[lightud]=nil
end

function rand(pcnt)
    return ( (mt.rand()*100) < pcnt)
end

function randnum(low, high)
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

function GetScript(subdir, name)
  if string.find(subdir, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end
  if string.find(name, "[^a-zA-Z0-9_]") then
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
		setmetatable=setmetatable,

		-- okay now our stuff
		-- checks
		hour=hour,

		-- other
        tprintstr=tprintstr,
        getroom=getroom,
		randnum=randnum,
		rand=rand,
		getobjproto=getobjproto,
		getobjworld=getobjworld,
		getmobworld=getmobworld,
        log=log,
        sendtochar=sendtochar,
        pagetochar=pagetochar,
        getcharlist=getcharlist,
        getmoblist=getmoblist,
        getplayerlist=getplayerlist
}
	
-- First look for main_lib funcs, then mob/area/obj funcs
-- (providing env as argument)
CH_env_meta={
    __index=function(tbl,key)
        if main_lib[key] then
            return main_lib[key]
        elseif tbl.mob[key] then 
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
        elseif tbl.obj[key] then
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
        elseif tbl.area[key] then
            return function(...)
                        table.insert(arg, 1, tbl.area) 
                        return tbl.area[key](unpack(arg)) 
                   end
        end
    end
}

function MakeEnvProxy(env)
    local proxy={}
    setmetatable(proxy, {
            __index = env,
            __newindex = function (t,k,v)
                if k=="tableid" then
                    error("Cannot alter tableid of environment.")
                else
                    t[k]=v
                end
            end }
    )

    return proxy
end

function new_AREA_env()
    local o={}
    setmetatable(o, AREA_env_meta)
    return o
end

function new_OBJ_env()
    local o={}
    setmetatable(o, OBJ_env_meta)
    return o
end

function new_CH_env(ud)
    local o={mob=ud}
    setmetatable(o, CH_env_meta)
    return MakeEnvProxy(o)
end

function mob_program_setup(ud, f)
    --print(tprintstr(ud))
    if envtbl[ud.tableid]==nil then
        envtbl[ud.tableid]=new_CH_env(ud) 
    end
    setfenv(f, envtbl[ud.tableid])
    return f
end

function obj_program_setup(ud, f)
    if ud.env==nil then
      rawset(ud, "env", new_OBJ_env())
      ud.env.obj=ud
      ud.env._G=ud.env
    end
    setfenv(f, ud.env)
    return f
end

function area_program_setup(ud, f)
    if ud.env==nil then
      rawset(ud, "env", new_AREA_env())
      ud.env.area=ud
      ud.env._G=ud.env
    end
    setfenv(f, ud.env)
    return f
end
