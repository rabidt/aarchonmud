local dbg=true

local mquests={}
local miniquestargs={}

local function miniquest_status( farg )
    local plr=getcharworld( farg.ch, farg.player)
    if plr==nil then
        return false, "Couldn't find " .. farg.player .. "."
    elseif isnpc(plr) then
        return false, farg.player .. " is an NPC!"
    end

    local quest=mquests[farg.name]
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

local function miniquest_show( farg )
    local quest=mquests[farg.name]
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

local function miniquests_list()
    if not(next(mquests)) then -- check if empty
        return false,"No miniquests."
    end

    local rtn=""

    for k,v in pairs(mquests) do
        rtn= rtn .. k .. "\n\r"
    end

    return true,rtn
end

local function miniquest_add( farg )
    if mquests[farg.name] then
        return false,"Name " .. farg.name .. " already exists."
    end

    mquests[farg.name]={}
    return true,"Miniquest added."
end

local function miniquest_remove( farg )
    if mquests[farg.name]==nil then
        return false,"Can't find " .. farg.name .. "."
    end

    mquests[farg.name]=nil
    return true,"Removed " .. farg.name .. "."
end

local function miniquest_mapqset( farg )
    if mquests[farg.name]==nil then
        return false,"Can't find " .. farg.name .. "."
    end
    
    if farg.description=="clear" then
        mquests[farg.name][farg.qset]=nil
        return true, "Cleared."
    else
        mquests[farg.name][farg.qset]=mquests[farg.name][farg.qset] or {}
        mquests[farg.name][farg.qset].description=farg.description
        return true, "Mapped."
    end
end

local function miniquest_mapvalue( farg )
    if mquests[farg.name]==nil then
        return false,"Can't find " .. "."
    end

    if not mquests[farg.name][farg.qset] then
        return false,"Must map the qset before you can map the values."
    end

    if farg.description=="clear" then
        mquests[farg.name][farg.qset][farg.value]=nil
        return true, "Cleared."
    else
        mquests[farg.name][farg.qset][farg.value]=farg.description
        return true, "Mapped."
    end
end

local function miniquest_help( farg )
    if not miniquestargs[farg.command] then
        return false, "No such command."
    else
        return true, miniquestargs[farg.command].help
    end
end

miniquestargs.help={
        args= {[1]={name="command", typ="string"} },
        fun=miniquest_help,
        help="miniquest help <command>"
    }

miniquestargs.list={
        args= {},
        fun=miniquest_list,
        help="Shows a list of the miniquests."
    }

miniquestargs.add={ 
        args= { [1]={name="name",typ="string"} }, 
        fun=miniquest_add,
        help="Add a miniquest to the list."
    }
    
miniquestargs.remove={ 
        args= { [1]={name="name",typ="string"} },
        fun=miniquest_remove,
        help="Remove a miniquest to the list."
    }

miniquestargs.show={
        args= { [1]={name="name",typ="string"} },
        fun=miniquest_show,
        help="Show the registered qsets and values for a miniquest."
    }

miniquestargs.mapqset={
        args= { [1]={name="name",typ="string"},
              [2]={name="qset",typ="number"},
              [3]={name="description",typ="string"} },
        fun=miniquest_mapqset,
        help="Register a qset as part of a miniquest and give it a description."
    }

miniquestargs.status={
        args= { [1]={name="player", typ="string"},
                [2]={name="name", typ="string"} },
        fun=miniquest_status,
        help="Show a player's status for a given miniquest based on current qsets."
    }

miniquestargs.mapvalue={
        args= { [1]={name="name",typ="string"},
              [2]={name="qset",typ="number"},
              [3]={name="value",typ="number"},
              [4]={name="description",typ="string"} },
        fun=miniquest_mapvalue,
        help="Give a description to a specific value for a qset in the miniquest."
    }

local function printhelptochar( ch )
    for k,v in pairs(miniquestargs) do
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
                                
function do_miniquest( ch, argument )
    local args={}
    for arg in string.gmatch(argument, "%S+") do
        table.insert(args, arg)
    end

    if #args<1 then
        printhelptochar(ch)
        return
    end
    
    local mqarg=miniquestargs[args[1]]
    table.remove(args,1) -- Won't need that anymore
    if not mqarg then
        printhelptochar(ch)
        return
    end

    local fun_args={ch=ch} --what we wil pass to the func
    for i,v in ipairs(mqarg.args) do
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

        if i<#mqarg.args then
            fun_args[v.name]=args[i]
        else
            -- Last arg grabs the rest
            fun_args[v.name]=table.concat(args, " ", i)
           
        end
    end

    -- Some magic to execute the function
    local rslt,msg
    rslt,msg=mqarg.fun(fun_args)

    if rslt==false then
        sendtochar(ch, "Command failed.\n\r")
    end
    if msg then
        sendtochar(ch,msg .. "\n\r")
    end
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
