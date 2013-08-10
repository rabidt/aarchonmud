local dbg=true

local mquests={}


local function show_miniquest( farg )
    local quest=mquests[farg.name]
    if quest==nil then
        return false,"Can't find " .. farg.name .. "."
    end

    local rtn=""
    for k,v in pairs(quest) do
        if type(k) == "number" then
            --rtn=rtn .. k .. " " .. v.description .. "\n\r"
            rtn=rtn .. string.format("%-8d %s\n\r", k, v.description)
            for l,w in pairs(v) do
                if type(l) == "number" then
                    --rtn=rtn .. l .. " " .. w .. "\n\r"
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

local function list_miniquests()
    if mquests=={} then
        return false,"No miniquests."
    end

    local rtn=""

    for k,v in pairs(mquests) do
        rtn= rtn .. k .. "\n\r"
    end

    return true,rtn
end

local function add_miniquest( farg )
    if mquests[farg.name] then
        return false,"Name " .. farg.name .. " already exists."
    end

    mquests[farg.name]={}
    return true,"Miniquest added."
end

local function remove_miniquest( farg )
    if mquests[farg.name]==nil then
        return false,"Can't find " .. farg.name .. "."
    end

    mquests[farg.name]=nil
    return true
end

local function map_qset( farg )
    if mquests[farg.name]==nil then
        return false,"Can't find " .. farg.name .. "."
    end
    
    --if dbg then return false, farg.name .. " " .. farg.qset .. " " .. farg.description end

    mquests[farg.name][farg.qset]=mquests[farg.name][farg.qset] or {}
    mquests[farg.name][farg.qset].description=farg.description
end

local function map_qset_value( farg )
    if mquests[farg.name]==nil then
        return false,"Can't find " .. "."
    end

    if not mquests[farg.name][farg.qset] then
        return false,"Must map the qset before you can map the values."
    end
    mquests[farg.name][farg.qset][farg.value]=farg.description

    return true
end

local miniquestargs = {
    list={
        args= {},
        fun=list_miniquests
    },

    add={ 
        args= { [1]={name="name",typ="string"} }, 
        fun=add_miniquest
    },
    
    remove={ 
        args= { [1]={name="name",typ="string"} },
        fun=remove_miniquest
    },

    show={
        args= { [1]={name="name",typ="string"} },
        fun=show_miniquest
    },

    mapqset={
        args= { [1]={name="name",typ="string"},
              [2]={name="qset",typ="number"},
              [3]={name="description",typ="string"} },
        fun=map_qset
    },

    mapvalue={
        args= { [1]={name="name",typ="string"},
              [2]={name="qset",typ="number"},
              [3]={name="value",typ="number"},
              [4]={name="description",typ="string"} },
        fun=map_qset_value
    }
}

local function printhelptochar( ch )
    for k,v in pairs(miniquestargs) do
        --sendtochar(ch, k .. " " .. #v.args .. "\n\r")
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
        --if dbg then sendtochar(ch, arg .. "\n\r") end
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

    local fun_args={} --what we wil pass to the func
    for i,v in ipairs(mqarg.args) do
        if args[i]==nil then
            sendtochar(ch, "Please provide argument for " .. v.name .. ".")
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
           
            --if dbg then print(fun_args[v.name] .. "\n\r") end
        end
    end

    -- Some magic to execute the function
    local rslt,msg
    if dbg then print(fun_args) end 
    rslt,msg=mqarg.fun(fun_args)

    if rslt==false then
        sendtochar(ch, "Command failed.\n\r")
    end
    if msg then
        sendtochar(ch,msg)
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
