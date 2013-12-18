envtbl=envtbl or {} -- game object script environments
origenv={} -- where the REAL env tables live

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
