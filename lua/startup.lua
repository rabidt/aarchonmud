package.path = mud.luadir() .. "?.lua"

require "tprint"
require "serialize"
require "utilities"
require "commands"

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

    table.insert(udtbl,ud)
    return
end

function UnregisterUd(lightud)
    for k,v in pairs (udtbl) do
        if v.tableid == lightud then
        -- DESTROY
            setmetatable(v,nil)
            rawset(v,"tableid",nil)
            v={}
            v=nil
        end
    end
end



function cleanup()
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

io=nil
os.execute=nil
os.rename=nil
os.remove=nil
os.exit=nil
os.setlocale=nil
