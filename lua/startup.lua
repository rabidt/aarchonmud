package.path = mud.luadir() .. "?.lua"

tprintstr=require "tprint"
require "serialize"
require "utilities"
require "leaderboard"

udtbl={} -- used to store game object tables, (read only proxies to origtbl)
envtbl={} -- game object script environments
origtbl={} -- where the REAL ud tables live
interptbl={} -- key is game object pointer, table of desc=desc pointer, name=char name

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
            __newindex = function (t,k,v)
                error("Cannot set values on game objects.")
            end,
            __tostring=function() return tostring(ud) end,
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
        setmetatable(origtbl[lightud], nil)
        origtbl[lightud]={}
        origtbl[lightud]=nil
        udtbl[lightud]={}
        udtbl[lightud]=nil
    end

    if envtbl[lightud] then
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
        getplayerlist=getplayerlist,
        getobjlist=getobjlist,
        getarealist=getarealist,
        clearloopcount=clearloopcount,
        god={confuse=god.confuse,
            curse=god.curse,
            plague=god.plague,
            bless=god.bless,
            slow=god.slow,
            speed=god.speed,
            heal=god.heal,
            enlighten=god.enlighten,
            protect=god.protect,
            fortune=god.fortune,
            haunt=god.haunt,
            cleanse=god.cleanse,
            defy=god.defy
        }

}
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
main_lib=ProtectLib(main_lib)

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
        else
            error("Invalid type in interp_setup: "..typ)
        end
    end

    interptbl[ud.tableid]={name=name, desc=desc}
    return 1,nil
end

function run_lua_interpret(env, str)
    local f,err=loadstring(str)
    if not(f) then
        error(err)
    end
    setfenv(f, env)
    f()
end

function wait_lua_interpret(env, str)
    interptbl[env.udid].buff=interptbl[env.udid] and interptbl[env.udid].buff or {}

    table.insert(interptbl[env.udid].buff, str)
end

function go_lua_interpret(env, str)
    local buff=interptbl[env.udid] and interptbl[env.udid].buff or {}

    if #buff>0 then
        run_lua_interpret(env, table.concat(buff, "\n"))
        interptbl[env.udid].buff=nil
    end
end
