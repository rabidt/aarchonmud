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


function GetAreaFname(env)
    local full
    if env.mob then
      full= env.mob.areafname
    elseif env.obj then
      full=env.obj.areafname
    elseif env.area then
      full=env.area.filename
    else
      error("Couldn't retrieve area filename.")
    end

    return string.match(full, "(%w+)\.are")
end

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

function savetbl( name, tbl, env)
  if string.find(name, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end
 
  local dir=GetAreaFname(env)
  if not os.rename(dir, dir) then
    os.execute("mkdir '" .. dir .. "'")
  end
  local f=io.open( dir .. "/" .. name .. ".lua", "w")
  out,saved=serialize.save(name,tbl)
  f:write(out)

  f:close()
end

function loadscript(subdir, name, env)
  if string.find(subdir, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end

  if string.find(name, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end


  os.execute("pwd")
  os.execute("ls")
  local fname = mud.userdir() .. subdir .. "/" .. name .. ".lua"
  local f,err=loadfile(fname)
  if f==nil then 
    error( fname .. "error: " ..  err) 
  end

  setfenv(f, env)
  return f()
end

function loadtbl(name,env)
  if string.find(name, "[^a-zA-Z0-9_]") then
    error("Invalid character in name.")
  end

  local dir=GetAreaFname(env)
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
		setmetatable=setmetatable,

		-- okay now our stuff
		-- checks
		hour=hour,
		ispc=ispc,
		isnpc=isnpc,
		isgood=isgood,
		isevil=isevil,
		isneutral=isneutral,
		isimmort=isimmort,
		ischarm=ischarm,
		isfollow=isfollow,
		isactive=isactive,
		isdelay=isdelay,
		isvisible=isvisible,
		hastarget=hastarget,
		istarget=istarget,
		affected=affected,
		act=act,
		off=off,
		imm=imm,
		carries=carries,
		wears=wears,
		has=has,
		uses=uses,
		name=name,
		qstatus=qstatus,
		vuln=vuln,
		res=res,
		skilled=skilled,
		ccarries=ccarries,
		qtimer=qtimer,
		canattack=canattack,

		-- other
		getroom=getroom,
		randnum=randnum,
		rand=rand,
		getobjproto=getobjproto,
		getobjworld=getobjworld,
		getmobworld=getmobworld,
		savetbl=savetbl,
		loadtbl=loadtbl,
        log=log,
        sendtochar=sendtochar
}
	
-- xxx_env_lib
-- These are for env specific functions
-- or common functions that need access
-- to env as a variable
CH_env_lib={
	loadprog=loadmprog,    
	loadscript=loadscript,
	tprint=function(tbl,env)
        local str={}
        if env.mob then
            tprint(str, tbl)
            env.mob:say(table.concat(str))
        end
	end
}

OBJ_env_lib={
	loadprog=loadoprog,
	loadscript=loadscript,
	tprint=function(tbl,env)
        local str={}
        if env.obj then
            tprint(str, tbl)
            env.obj:echo(table.concat(str))
        end
    end
}

AREA_env_lib={
	loadprog=loadaprog,
	loadscript=loadscript,
	tprint=function(tbl,env)
        local str={}
        if env.area then
            tprint(str, tbl)
            env.area:echo(table.concat(str))
        end
    end
}

-- First look for mob functions, then look in main_lib, then look in env_lib
-- (providing env as argument)
CH_env_meta={
    __index=function(tbl,key)
        if tbl.mob[key] then 
            return function(...) tbl.mob[key](tbl.mob, unpack(arg)) end
        elseif main_lib[key] then
			return main_lib[key]
		else
            return function(...) CH_env_lib[key](unpack(arg), tbl) end 
        end
    end
}

OBJ_env_meta={
    __index=function(tbl,key)
        if tbl.obj[key] then
            return function(...) tbl.obj[key](tbl.obj, unpack(arg), tbl) end
        elseif main_lib[key] then
			return main_lib[key]
		else
            return function(...) OBJ_env_lib[key](unpack(arg), tbl) end
        end
    end
}

AREA_env_meta={
    __index=function(tbl,key)
        if tbl.area[key] then
            return function(...) tbl.area[key](tbl.area, unpack(arg), tbl) end
        elseif main_lib[key] then
			return main_lib[key]
		else
            return function(...) AREA_env_lib[key](unpack(arg), tbl) end
        end
    end
}

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
    setfenv(f, ud.env)
    return f
end

function obj_program_setup(ud, f)
    if ud.env==nil then
      rawset(ud, "env", new_OBJ_env())
      ud.env.obj=ud
    end
    setfenv(f, ud.env)
    return f
end

function area_program_setup(ud, f)
    if ud.env==nil then
      rawset(ud, "env", new_AREA_env())
      ud.env.area=ud
    end
    setfenv(f, ud.env)
    return f
end
