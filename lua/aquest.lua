local dbg=true

local mquests={}
local aquestargs={}

local function aquest_status( quests, farg )
    local plr=getcharworld( farg.ch, farg.player)
    if plr==nil then
        return false, "Couldn't find " .. farg.player .. "."
    elseif isnpc(plr) then
        return false, farg.player .. " is an NPC!"
    end

    local quest=quests[farg.name]
    if quest==nil then
        return false,"Can't find " .. farg.name .. "."
    end

    local rtn=""
    for k,v in pairs(quest) do
        if type(k) == "number" then
            rtn=rtn .. string.format("%-8d %s\n\r", k, v.description)
            local val=qstatus(plr, k)
            if dbg then print(val) end
            if val==0 then 
                rtn=rtn .. "    0\n\r"
            else
                for l,w in pairs(v) do
                    if l==val then
                        rtn=rtn .. string.format("    %-8d %s\n\r", l, w)
                    end
                end
            end
        end
    end
                   
    return true, rtn
end

local function aquest_show( quests, farg )
    local quest=quests[farg.name]
    if quest==nil then
        return false,"Can't find " .. farg.name .. "."
    end

    local rtn=""
    for k,v in pairs(quest) do
        if type(k) == "number" then
            rtn=rtn .. string.format("%-8d %s\n\r", k, v.description)
            for l,w in pairs(v) do
                if type(l) == "number" then
                    rtn=rtn .. string.format("    %-8d %s\n\r", l, w)
                end
            end
        end
    end

    if rtn=="" then
        return false, "Nothing mapped."
    end

    return true,rtn
end

local function aquest_list(quests)
    if not(next(quests)) then -- check if empty
        return false,"No quests."
    end

    local rtn=""

    for k,v in pairs(quests) do
        rtn= rtn .. k .. "\n\r"
    end

    return true,rtn
end

local function aquest_add( quests, farg )
    if quests[farg.name] then
        return false,"Name " .. farg.name .. " already exists."
    end

    quests[farg.name]={}
    return true,"Miniquest added."
end

local function aquest_remove( quests, farg )
    if quests[farg.name]==nil then
        return false,"Can't find " .. farg.name .. "."
    end

    quests[farg.name]=nil
    return true,"Removed " .. farg.name .. "."
end

local function aquest_mapqset( quests, farg )
    if quests[farg.name]==nil then
        return false,"Can't find " .. farg.name .. "."
    end
    
    if farg.description=="clear" then
        quests[farg.name][farg.qset]=nil
        return true, "Cleared."
    else
        quests[farg.name][farg.qset]=quests[farg.name][farg.qset] or {}
        quests[farg.name][farg.qset].description=farg.description
        return true, "Mapped."
    end
end

local function aquest_mapvalue( quests, farg )
    if quests[farg.name]==nil then
        return false,"Can't find " .. "."
    end

    if not quests[farg.name][farg.qset] then
        return false,"Must map the qset before you can map the values."
    end

    if farg.description=="clear" then
        quests[farg.name][farg.qset][farg.value]=nil
        return true, "Cleared."
    else
        quests[farg.name][farg.qset][farg.value]=farg.description
        return true, "Mapped."
    end
end

local function aquest_help( quests, farg )
    if not aquestargs[farg.command] then
        return false, "No such command."
    else
        return true, aquestargs[farg.command].help
    end
end

aquestargs.help={
        args= {[1]={name="command", typ="string"} },
        fun=aquest_help,
        help="help <command>"
    }

aquestargs.list={
        args= {},
        fun=aquest_list,
        help="Shows a list of the quests."
    }

aquestargs.add={ 
        args= { [1]={name="name",typ="string"} }, 
        fun=aquest_add,
        help="Add a quest to the list."
    }
    
aquestargs.remove={ 
        args= { [1]={name="name",typ="string"} },
        fun=aquest_remove,
        help="Remove a quest to the list."
    }

aquestargs.show={
        args= { [1]={name="name",typ="string"} },
        fun=aquest_show,
        help="Show the registered qsets and values for a aquest."
    }

aquestargs.mapqset={
        args= { [1]={name="name",typ="string"},
              [2]={name="qset",typ="number"},
              [3]={name="description",typ="string"} },
        fun=aquest_mapqset,
        help="Register a qset as part of a quest and give it a description."
    }

aquestargs.status={
        args= { [1]={name="player", typ="string"},
                [2]={name="name", typ="string"} },
        fun=aquest_status,
        help="Show a player's status for a given quest based on current qsets."
    }

aquestargs.mapvalue={
        args= { [1]={name="name",typ="string"},
              [2]={name="qset",typ="number"},
              [3]={name="value",typ="number"},
              [4]={name="description",typ="string"} },
        fun=aquest_mapvalue,
        help="Give a description to a specific value for a qset in the quest."
    }

local function printhelptochar( ch )
    for k,v in pairs(aquestargs) do
        local args=""
        for i,w in ipairs(v.args) do
            args=args .. w.name
            if not(i==#v.args) then
                args=args .. ", "
            end
        end
        sendtochar(ch,
                string.format("%-15s %-35s\n\r", k, args) ) 
    end
end
                                
local function aquest_command( ch, argument, quests) 
    local args={}
    for arg in string.gmatch(argument, "%S+") do
        table.insert(args, arg)
    end

    if #args<1 then
        printhelptochar(ch)
        return
    end
    
    local aqarg=aquestargs[args[1]]
    table.remove(args,1) -- Won't need that anymore
    if not aqarg then
        printhelptochar(ch)
        return
    end

    local fun_args={ch=ch} --what we wil pass to the func
    for i,v in ipairs(aqarg.args) do
        if args[i]==nil then
            sendtochar(ch, "Please provide argument for " .. v.name .. ".\n\r")
            return
        end

        if v.typ=="number" then
            args[i]=tonumber(args[i])
            if args[i]==nil then
                sendtochar(ch, v.name .. " needs to be a number.\n\r")
                return
            end
        elseif v.typ=="string" then
            if args[i]=="" then
                sendtochar(ch, v.name .. " needs to be a string.\n\r")
                return
            end 
        end

        if i<#aqarg.args then
            fun_args[v.name]=args[i]
        else
            -- Last arg grabs the rest
            fun_args[v.name]=table.concat(args, " ", i)
           
        end
    end

    -- Some magic to execute the function
    local rslt,msg
    rslt,msg=aqarg.fun(quests, fun_args)

    if rslt==false then
        sendtochar(ch, "Command failed.\n\r")
    end
    if msg then
        sendtochar(ch,msg .. "\n\r")
    end
end

function do_miniquest( ch, argument )
    aquest_command( ch, argument, mquests)
end

function do_aquest( ch, argument )
    --logtprint(ud_tbl)

    --local ar=ch.room.area

    --logtprint(udtbl)
    aquest_command( ch, argument, ch.room.area.env.area_quests )
end

function save_miniquests()
  local f=io.open( "miniquests.lua", "w")
  out,saved=serialize.save("mquests",mquests)
  f:write(out)

  f:close()
end

function load_miniquests()
  local f=loadfile("miniquests.lua")
  if f==nil then
    return
  end

  mquests=f()
end

function save_area_quests()
    save_miniquests()

    local f=function()
        if not(next(area_quests)) then return end -- check if empty
        savetbl("area_quests", area.env.area_quests, area.env)
    end        
    
    for k,v in pairs(getareas()) do
        area_program_setup(v, f)
        f()
    end
end

function load_areaquests()
    load_miniquests()

    local f=function()
        area.env.area_quests=loadtbl("area_quests", area.env) or {} 
    end

    for k,v in pairs(getareas()) do
        area_program_setup(v, f)
        f()
    end
end
