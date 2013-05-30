local dbg=true
local lb_tables={
    daily={
        timeout=0,
        tables={
            mkill={name="Mob Kills", entries={} },
            qcomp={name="Quest Completed", entries={} },
            qpnt={name="Quest Points", entries={} },
            qfail={name="Quests Failed", entries={} },
            level={name="Levels Gained", entries={} } 
        } 
    },
    weekly={
        timeout=0,
        tables={
            mkill={name="Mob Kills", entries={} },
            qcomp={name="Quest Completed", entries={} },
            qpnt={name="Quest Points", entries={} },
            qfail={name="Quests Failed", entries={} },
            level={name="Levels Gained", entries={} } 
        } 
    },
    monthly={
        timeout=0,
        tables={
            mkill={name="Mob Kills", entries={} },
            qcomp={name="Quest Completed", entries={} },
            qpnt={name="Quest Points", entries={} },
            qfail={name="Quests Failed", entries={} },
            level={name="Levels Gained", entries={} } 
        } 
    },
    overall={
        timeout=0,
        tables={
            mkill={name="Mob Kills", entries={} },
            qcomp={name="Quest Completed", entries={} },
            bhd={name="Beheads", entries={} },
            wkill={name="War Kills", entries={} },
            expl={name="Rooms Explored", entries={} },
            qfail={name="Quests Failed", entries={} },
            pkill={name="Player Kills", entries={} }
        } 
    }
}

local types=
{
    [0]="mkill",
    [1]="qcomp",
    [2]="bhd",
    [3]="qpnt",
    [4]="wkill",
    [5]="expl",
    [6]="qfail",
    [7]="level",
    [8]="pkill"
}


local function find_in_lboard(ent, chname)
    local ind=nil
    for i,v in ipairs(ent) do
      if v.chname == chname then
        ind=i
        break
      end
    end

    return ind
end

local function update_lboard_periodic(ent, chname, increment)
    if dbg then print("update_lboard_periodic") end
    local ind=find_in_lboard(ent, chname)
    if not ind then 
        table.insert(ent, {chname=chname, value=increment})
    else
        ent[ind].value=ent[ind].value+increment
    end

    table.sort(ent, function(a,b) return a.value>b.value end)
end

local function update_lboard_overall(ent, chname, current)
    local ind=find_in_lboard(ent, chname)
    if not ind then
        if #ent>=20 and ent[#ent].value>current then return end
        table.insert(ent, {chname=chname, value=current})
    else
        ent[ind].value=current
    end

    table.sort(ent, function(a,b) return a.value>b.value end)
    while #ent>20 do
      table.remove(ent,21)
    end
end

function update_lboard( typ, chname, current, increment)
    if dbg then print("update_lboard") end
    for k,tbl in pairs(lb_tables) do
        
        if tbl.tables[types[typ]] then
            if k=="overall" then
                update_lboard_overall(tbl.tables[types[typ]].entries , chname, current)
            else
                update_lboard_periodic(tbl.tables[types[typ]].entries, chname, increment)
            end
        end
    end
end

function remove_from_all_lboards(chname)
  for k,tbl in lb_tables do
    for l,tb in tbl.tables do
      for i,v in ipairs(tb) do
        if v.chname==chname then
          table.remove(tb, i)
          break
        end
      end
    end
  end
end


function save_lboards()
  local f=io.open( "lboard.lua", "w")
  out,saved=serialize.save("lb_tables",lb_tables)
  f:write(out)

  f:close()
end

function load_lboards()
  local f=loadfile("lboard.lua")
  if f==nil then
    return
  end

  lb_tables=f()
  check_lboard_reset()
end
    
local function print_entries(entries, ch, maxent)
  local found=false
  local cnt=1
  for ind, entry in ipairs(entries) do
    if entry.chname==ch.name then
      sendtochar(ch,string.format("{W%3d: %-25s %10d{x\r\n", ind,entry.chname, entry.value))
      found=true
    else
      sendtochar(ch,string.format("%3d: %-25s %10d\r\n", ind,entry.chname, entry.value))
    end
    cnt=cnt+1
    if cnt>maxent then break end
  end

  return found
end

function do_lboard( ch, argument)
    local interval_args={ "daily", "weekly", "monthly", "overall" }
    -- types represents type args
    local interval
    local typ
    local args=string.gmatch(argument, "%S+")
    -- Parse the arguments
    for v in args do
      local found
      for _,w in pairs(interval_args) do
        if string.find(w, v) then
          interval=w
          found=true
          break
        end
      end
      if not found then
        for _,w in pairs(types) do
          if string.find(w, v) then
            typ=w
            break
          end
        end
      end
    end

    -- print applicable tables
    for k,tbl in pairs(lb_tables) do
      if interval==nil or interval==k then
        sendtochar(ch, k .. "\r\n")
        if not(k=="overall") then
          sendtochar(ch, os.date("%X %x ",tbl.timeout) .. "\r\n")
        end

        for l,tb in pairs(tbl.tables) do
          if typ==nil or typ==l then
            sendtochar(ch, tb.name .. "\r\n")
            sendtochar(ch, string.format("Rank %-25s %10s\r\n", "Player", "Value"))
            local found=print_entries(tb.entries, ch, 20)
            if not found then
              ind=find_in_lboard(tb.entries,ch.name)
              if ind then
                sendtochar(ch, string.format("{W%3d: %-25s %10d{x\r\n", ind, tb.entries[ind].chname, tb.entries[ind].value))
              else
                sendtochar(ch, "No score for " .. ch.name .. "\r\n")
              end
            end
          end
        end
      end
    end

    return

end

local function reset_daily()
  local now=os.date("*t")
  lb_tables.daily.timeout=os.time{year=now.year, month=now.month,day=now.day+1}
end

local function reset_weekly()
  local now=os.date("*t")
  lb_tables.weekly.timeout=os.time{year=now.year, month=now.month,day=now.day+8-now.wday}
end

local function reset_monthly()
  local now=os.date("*t")
  lb_tables.monthly.timeout=os.time{year=now.year, month=now.month+1,day=now.day}
end

function check_lboard_reset()
  for k,tbl in pairs(lb_tables) do
    if os.time()>tbl.timeout then
      if k=="overall" then
      elseif k=="daily" then
        reset_daily()
      elseif k=="weekly" then
        reset_weekly()
      elseif k=="monthly" then
        reset_monthly()
      end
    end
  end
end
        


