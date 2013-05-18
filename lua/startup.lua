package.path = mud.luadir() .. "?.lua"

require "tprint"
require "serialize"
require "utilities"

function send_nocr (...)
    say (table.concat {...})
end -- send_nocr

function send (...)
    say (table.concat {...})
end

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

CH_env_meta={
    __index=_G
}

setmetatable(CH_env_meta, CH_env_lib)

os.execute=nil
os.rename=nil
os.remove=nil
os.exit=nil
os.setlocale=nil
