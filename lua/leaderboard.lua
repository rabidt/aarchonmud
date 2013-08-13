local dbg=true

local function new_lbtables(typ)
    local rtn={}
    if typ=="daily" then
        rtn={mkill={name="Mob Kills", entries={} },
            qcomp={name="Quest Completed", entries={} },
            qpnt={name="Quest Points", entries={} },
            qfail={name="Quests Failed", entries={} },
            level={name="Levels Gained", entries={} }}
    elseif typ=="weekly" then
        rtn={mkill={name="Mob Kills", entries={} },
            qcomp={name="Quest Completed", entries={} },
            qpnt={name="Quest Points", entries={} },
            qfail={name="Quests Failed", entries={} },
            level={name="Levels Gained", entries={} }}
    elseif typ=="monthly" then
        rtn={mkill={name="Mob Kills", entries={} },
            qcomp={name="Quest Completed", entries={} },
            qpnt={name="Quest Points", entries={} },
            qfail={name="Quests Failed", entries={} },
            level={name="Levels Gained", entries={} }}
    elseif typ=="overall" then
        rtn={mkill={name="Mob Kills", entries={} },
            qcomp={name="Quest Completed", entries={} },
            bhd={name="Beheads", entries={} },
            wkill={name="War Kills", entries={} },
            expl={name="Rooms Explored", entries={} },
            qfail={name="Quests Failed", entries={} },
            pkill={name="Player Kills", entries={} }}
    else
        error("new_lbtables, invalid typ argument")
    end

    return rtn
end

local lb_tables={
    daily={
        timeout=0,
        tables=new_lbtables("daily")
    },
    weekly={
        timeout=0,
        tables=new_lbtables("weekly")
    },
    monthly={
        timeout=0,
        tables=new_lbtables("monthly")
    },
    overall={
        timeout=0,
        tables=new_lbtables("overall")
    }
}

local lh_tables={
    daily={},
    weekly={},
    monthly={}
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

  -- Save lboard history too
  f=io.open( "lhistory.lua", "w")
  out,saved=serialize.save("lh_tables",lh_tables)
  f:write(out)

  f:close()
end

function load_lboards()
  local f=loadfile("lboard.lua")
  if f==nil then
    return
  end

  lb_tables=f()

  -- Load lboard history too
  f=loadfile("lhistory.lua")
  if f==nil then
    return
  end

  lh_tables=f()

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

local function print_tables(tables, ch, typefilter)
    for ttyp,tbl in pairs(tables) do
        if typefilter==nil or typefilter==ttyp then
            sendtochar(ch, tbl.name .. "\r\n")
            if not(print_entries(tbl.entries,ch, 20)) then
              local ind=find_in_lboard(tbl.entries,ch.name)
              if ind then
                sendtochar(ch, string.format("{W%3d: %-25s %10d{x\r\n", ind, tbl.entries[ind].chname, tbl.entries[ind].value))
              else
                sendtochar(ch, "No score for " .. ch.name .. "\r\n") 
              end
            end
        end
    end
end

local function print_interval(intrvl, ch, typefilter)
    if not(intrvl.timeout==0) then
        sendtochar(ch, "Ending: " .. os.date("%X %x ",intrvl.timeout) .. "\r\n")
    end

    print_tables(intrvl.tables,ch,typefilter)
end

function do_lhistory( ch, argument)
    local interval_args={ "daily", "weekly", "monthly", "overall" }
    -- types represents type args
    local interval
    local typ=nil
    local ind=nil
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
            found=true
            break
          end
        end
      end
      if not found then
        if not(tonumber(v)==nil) then
          ind=tonumber(v)
          found=true
          break
        end
      end 
    end


    -- print applicable tables
    for k,tbl in pairs(lh_tables) do
      if interval==nil or interval==k then
        sendtochar(ch, k:upper() .. " LEADERBOARD HISTORY\n\r")
        for i,v in ipairs(tbl) do
          if ind==nil or ind==i then
            sendtochar(ch, "Entry #" .. i .. "\n\r")
            print_interval(v, ch, typ)
            sendtochar(ch, "\n\r")
          end
        end
      end
    end

    return 
end

function do_lboard( ch, argument)
    local interval_args={ "daily", "weekly", "monthly", "overall" }
    -- types represents type args
    local interval
    local typ=nil
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
        sendtochar(ch, k:upper() .. " LEADERBOARD\n\r")
        print_interval(tbl, ch, typ)
        sendtochar(ch, "\n\r")
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

local function make_result(tbl,typ)
    local rslt={}
    rslt.timeout=tbl.timeout
    rslt.tables=tbl.tables

    tbl.tables=nil
    tbl.tables=new_lbtables(typ)

    local tmp=lh_tables[typ] or {}
    table.insert(tmp, 1, rslt)
    lh_tables[typ]=tmp
    
    while #lh_tables[typ]>10 do
        table.remove(lh_tables[typ],#lh_tables[typ])
    end
end

function check_lboard_reset()
  for k,tbl in pairs(lb_tables) do
    if os.time()>tbl.timeout and not(tbl.timeout==0) then
      if k=="daily" then
        make_result(tbl,k)
        reset_daily()
      elseif k=="weekly" then
        make_result(tbl,k)
        reset_weekly()
      elseif k=="monthly" then
        make_result(tbl,k)
        reset_monthly()
      end
    end
  end
end
        


