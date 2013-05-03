systeminfo=mud.system_info()
package.path = systeminfo.LUA_DIR .. "?.lua"

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

    table.insert(udtbl,ud)
    return
end

function cleanup()
    if udtbl==nil then
        return
    end

    -- DESTROY
    for _,v in ipairs (udtbl) do
        setmetatable(v,nil)
        rawset(v,"tableid",nil) 
        v=nil
    end
    udtbl={}
end

function same(tbl1, tbl2)
    if tbl1==nil then
        return false
    elseif tbl2==nil then
        return false
    end

    if tbl1.tableid~= nil and tbl2~=nil and tbl1.tableid==tbl2.tableid then
      return true
    else
      return false
    end
end

