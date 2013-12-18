interptbl=interp or {} -- key is game object pointer, table of desc=desc pointer, name=char name

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