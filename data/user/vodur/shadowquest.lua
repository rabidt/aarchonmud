
if shared.shadowquest then return end
-- Force to load in area env to prevent funkiness with shared table
-- binding to stale envs
do
  local area = getroom(4700).area
  if not(self == area) then
    area:loadscript("vodur", "shadowquest")
    return
  end
end
local shadowquest = {}

do
  local exists
  for cnt in db:urows([[
    SELECT count(*) FROM sqlite_master WHERE type='table' AND name = 'shadowquest'
    ]])
  do
    exists = ( cnt > 0 )
  end

  if not(exists) then
    log("Creating and populating shadowquest tables in script db.")

    local res = db:exec([[
      CREATE TABLE shadowquest(
        player_name TEXT,
        shadow_kills INT,
        shadow_rwd INT,
        darkness_kills INT,
        darkness_rwd INT,
        darkness_timer INT,
        group_kills INT,
        group_timer INT
      );]])

    assert(sqlite3.OK == res)

    log("Done.")
  end
end

local check_player_init = (function()
  local st_check = assert(db:prepare([[
    SELECT *
    FROM shadowquest
    WHERE
    player_name = ?
    ]]))
  local st_init = assert(db:prepare([[
    INSERT INTO shadowquest(
      player_name,
      shadow_kills,
      shadow_rwd,
      darkness_kills,
      darkness_rwd,
      darkness_timer,
      group_kills,
      group_timer)
      VALUES (?,0,0,0,0,0,0,0)
    ]]))

  return function( ch, init )
    st_check:reset()
    st_check:bind_values(ch.name)
    
    for entry in st_check:nrows() do
      return entry
    end

    if not(init) then return nil end

    -- If we get here it's not in the table yet
    st_init:reset()
    st_init:bind_values(ch.name)
    assert(sqlite3.DONE == st_init:step())

    -- query again, should be there now
    st_check:reset()
    st_check:bind_values(ch.name)
    
    for entry in st_check:nrows() do
        return entry
    end
  end
end)()

-- return entry if exists, nil if not
local function get_player_entry( ch )
  return check_player_init( ch, false )
end

local incr_shadow_kills = (function()
  local st = assert(db:prepare([[
    UPDATE shadowquest 
    SET shadow_kills = shadow_kills + 1
    WHERE player_name = ?
    ]]))

  return function( ch )
    check_player_init( ch, true )
    st:reset()
    st:bind_values(ch.name)
    assert(sqlite3.DONE == st:step())
  end
end)()

local add_shadow_rwd = (function() 
  local st = assert(db:prepare([[
    UPDATE shadowquest
    SET shadow_rwd = shadow_rwd + ?
    WHERE player_name = ?
    ]]))
  return function( ch, amount )
    check_player_init( ch, true )
    st:reset()
    st:bind_values(amount, ch.name)
    assert(sqlite3.DONE == st:step())
  end
end)()

local incr_darkness_kills = (function()
  local st = assert(db:prepare([[
    UPDATE shadowquest 
    SET darkness_kills = darkness_kills + 1
    WHERE player_name = ?
    ]]))
  return function( ch )
    check_player_init( ch, true )
    st:reset()
    st:bind_values(ch.name)
    assert(sqlite3.DONE == st:step())
  end
end)()

local add_darkness_rwd = (function() 
  local st = assert(db:prepare([[
    UPDATE shadowquest
    SET darkness_rwd = darkness_rwd + ?
    WHERE player_name = ?
    ]]))
  return function( ch, amount )
    check_player_init( ch, true )
    st:reset()
    st:bind_values(amount, ch.name)
    assert(sqlite3.DONE == st:step())
  end
end)()

local set_darkness_timer = (function()
  local st = assert(db:prepare([[
    UPDATE shadowquest
    SET darkness_timer = ?
    WHERE player_name = ?
    ]]))
  return function( ch, amount )
    check_player_init( ch, true )
    st:reset()
    st:bind_values(amount, ch.name)
    assert(sqlite3.DONE == st:step())
  end
end)()

local incr_group_kills = (function()
  local st = assert(db:prepare([[
    UPDATE shadowquest 
    SET group_kills = group_kills + 1
    WHERE player_name = ?
    ]]))
  return function( ch )
    check_player_init( ch, true )
    st:reset()
    st:bind_values(ch.name)
    assert(sqlite3.DONE == st:step())
  end
end)()

local set_group_timer = (function()
  local st = assert(db:prepare([[
    UPDATE shadowquest
    SET group_timer = ?
    WHERE player_name = ?
    ]]))
  return function( ch, amount )
    check_player_init( ch, true )
    st:reset()
    st:bind_values(amount, ch.name)
    assert(sqlite3.DONE == st:step())
  end
end)()

local cntrl_place_shadows -- forward declare

local cntrl_say_cmd = {
  timers = {
      desc = "Show countdown for players",
      func = (function()
        local st = assert(db:prepare([[
          SELECT player_name, darkness_timer, group_timer
          FROM shadowquest]]))
        return function(mob, ch, trigger)
          local tm = os.time()
          mob:say("displaying timers")
          local out = {}
          table.insert(out, string.format(
            "%-20s | darkn| group",
            " "))
          st:reset()
          for row in st:nrows() do
            table.insert(out, string.format(
              "%-20s |%-5d | %-5d",
              row.player_name,
              math.max(0, row.darkness_timer - tm),
              math.max(0, row.group_timer - tm)
              ))
          end

          pagetochar(ch, table.concat(out, "\n\r").."\n\r")
        end
      end)()
  },

  place = {
    desc = "Force a shadow placement.",
    func = function(mob, ch, trigger)
      cntrl_place_shadows(mob)
    end
  },

  unload = {
    desc = "unload shadowquest shared table",
    func = function(mob, ch, trigger)
      shared.shadowquest = nil
      mob:say("unloaded")
    end
  },
  reload = {
    desc = "reload shadowquest.lua",
    func = function(mob, ch, trigger)
      shared.shadowquest = nil
      mob:loadscript("vodur", "shadowquest")
      mob:say("reloaded")
    end
  },
  track = {
    desc = "show tracking for all players",
    func = (function()
      local st = assert(db:prepare([[
        SELECT * 
        FROM shadowquest]]))
      return function(mob, ch, trigger)
        local out = {}
        table.insert(out,
          string.format("%-20s |%-13s |%-13s |%s",
            "name", "shadows", "darkness", "group"))
        st:reset()
        for row in st:nrows() do
          table.insert(out,
            string.format("%-20s |%5d %5dqp |%5d %5dqp |%5d", 
              row.player_name,
              row.shadow_kills,
              row.shadow_rwd,
              row.darkness_kills,
              row.darkness_rwd,
              row.group_kills))
        end

        pagetochar(ch, table.concat(out, "\n\r").."\n\r")
      end
    end)()
  }
}

local function cntrl_say(mob, ch, trigger)
  if trigger=="list" then
    for k,v in pairs(cntrl_say_cmd) do
      mob:say("%-8s - %s", k, v.desc)
    end
  elseif cntrl_say_cmd[trigger] then
    cntrl_say_cmd[trigger].func(mob, ch, trigger)
  end

  return
end

local VM_SHADOW = 4723 -- mysterious shadow
local VM_DARKNESS = 4702 -- shadowy darkness
local VM_PRIEST = 4726 -- shadow priest
local VM_COLOSSUS = 4724 -- shadow colossus
local VM_SMOKE = 4725 -- black smoke monster
local VM_CNTRL = 4722 -- controller mob

local VO_CRYSTAL = 4748 -- create portal to shadowy darkness
local VO_PORTAL = 4749 -- portal to shadowy darkness
local VO_SCROLL = 4750 -- scroll of charred skin
local VO_PIT_PORTAL = 4751 -- portal to pit
local VO_SPHERE = 4752 -- create pit portal

local VR_GROUP_FIGHT = 4703 -- room where gorup fight happens

local DLY_PLACE_SHADOWS = 300 -- 5 mins
local DLY_CHECK_CRYSTAL = 360 -- 6 mins
local DLY_CHECK_SPHERE = 720 -- 12 mins
local DLY_DARKNESS = 1800 -- 30 mins
local DLY_GROUP_FIGHT = 7200 -- 2 hours

local MAX_SHADOW_KILL = 20
local MAX_DARKNESS_KILL = 10
local MAX_GROUP_KILL = 5 

local function cntrl_check_crystals(mob)
  local lst = getplayerlist()
  local currtime = os.time()

  for _,plr in pairs(lst) do
    local entry = get_player_entry(plr)

    if entry
      and entry.shadow_kills >= MAX_SHADOW_KILL 
      and entry.darkness_kills < MAX_DARKNESS_KILL
      and entry.darkness_timer < currtime
      and not(plr.room.area:flag("remort"))
      and not(plr:carries(VO_CRYSTAL))
    then
      plr:oload(VO_CRYSTAL)
      sendtochar(plr, "A black crystal appears in your inventory.\n\r")
      mob:say("gave crystal to %s", plr.name)
    end
  end          
end

-- forward declared above
cntrl_place_shadows = function(mob)
  local pl = getplayerlist()

  for _,plr in pairs(pl) do
    local entry = get_player_entry(plr)
    local shadow_kills = entry and entry.shadow_kills or 0

    if plr.level < 101
      and not(plr.room:flag("safe"))
      and not(plr.room.vnum == 2) -- Limbo
      and not(plr.room.area:flag("remort"))
      and rand(66)
      and not(plr.room.vnum==4702 or plr.room.vnum==4703)
      and shadow_kills < MAX_SHADOW_KILL 
    then
      local room = plr.room
      local war = room:mload(VM_SHADOW)
      war.level = math.max(1, plr.level + (shadow_kills/2) + randnum(-5,5))
      if shadow_kills > 10 then
        war:kill(plr)
      end
      room:echo("%s appears seemingly from nowhere at the corner of your vision.", war.shortdescr)
      mob:say("placed level %d in %d with  %s", war.level, room.vnum, plr.name)
    end
  end
end

local function cntrl_check_spheres(mob)
  local lst = getplayerlist()
  local currtime = os.time()

  for _,plr in pairs(lst) do
    local entry = get_player_entry(plr)
    
    if entry 
      and entry.darkness_kills >= MAX_DARKNESS_KILL 
      and entry.group_kills < MAX_GROUP_KILL
      and entry.group_timer < currtime
      and not(plr.room.area:flag("remort"))
      and not(plr:carries(VO_SPHERE))
    then
      plr:oload(VO_SPHERE)
      sendtochar(plr, "{DThe shadow priest speaks into your mind.\n\r{x")
      sendtochar(plr, "%s, you have had much conquest against my minions.\n\r", plr.name)
      sendtochar(plr, "When you are ready for a greater challenge, drop this globe and enter the abyss.\n\r")
      sendtochar(plr, "However, you must bring a partner. There must be 2, no more, no less.\n\r")
      mob:say("gave sphere to %s", plr.name)
    end
  end
end

local function cntrl_timer(mob)
  local currtime = os.time()
  shad_time = shad_time or 0
  check_port_time = check_port_time or 0
  check_obstime = check_obstime or 0


  if (currtime > check_port_time) then
    mob:say("sending crystal")
    cntrl_check_crystals(mob)
    mob:say("done")
    check_port_time = currtime + DLY_CHECK_CRYSTAL
  end

  if (currtime > shad_time) then
    mob:say("placing shadows")
    cntrl_place_shadows(mob)
    mob:say("place done")
    shad_time = currtime + DLY_PLACE_SHADOWS
  end

  if (currtime > check_obstime) then
    mob:say("sending spheres")
    cntrl_check_spheres(mob)
    mob:say("done")
    check_obstime = currtime + DLY_CHECK_SPHERE
  end
end

function shadowquest.cntrl_trigger(mob, ch, trigger, trigtype)
  if trigtype == "timer" then
    return cntrl_timer(mob)
  elseif trigtype == "speech" then
    return cntrl_say(mob, ch, trigger)
  end
end

function shadowquest.shadow_death(mob, ch)
  if ch.isnpc then return end

  local entry = get_player_entry(ch)
  if entry and entry.shadow_kills >= MAX_SHADOW_KILL then
      return
  end

  local rwd = randnum(3,8)
  mob:reward(ch, "qp", rwd)

  incr_shadow_kills(ch)
  add_shadow_rwd(ch, rwd)
end

local function darkness_death(mob, ch, trigger)
  if ch.isnpc then return end

  -- record and reward
  local rwd = randnum(15, 35)
  mob:reward(ch, "qp", rwd)

  incr_darkness_kills(ch)
  add_darkness_rwd(ch, rwd)
  set_darkness_timer(ch, os.time() + DLY_DARKNESS)

  local ctrl = getmobworld(VM_CNTRL)[1]
  if ctrl then ctrl:say("shadowy darkness killed by %s", ch.name) end

    -- clean up
  mob:act("see_all")
  ch:goto(10204)
  echoaround(ch, "A shadowy portal appears suddenly, dropping %s into the square, then disappearing again.", ch.name)
  local newmob = mob.room:mload(mob.vnum)
end

local function darkness_grall(mob, ch, trigger)
    if ch.isnpc then return end

    -- Find out how many kills
    local entry = get_player_entry(ch)
    local kills = entry and entry.darkness_kills or 0
    
    if kills >= MAX_DARKNESS_KILL then
      ch:goto(10204)
      ch:mdo("look")
      sendtochar(ch, "You are immediately cast from the darkness. Guess it doesn't like you much.\n\r")
      ch:echoaround(ch, "A shadowy portal appears suddenly, dropping %s into the square, then disappearing again.", ch.name)
      return
    end

    -- Kill all the portals
    for k,v in pairs(getobjworld(VO_PORTAL)) do
        v.room:echo("%s fades into nothingness.", v.shortdescr)
        v:destroy()
    end

    -- Vis up and kill
    mob:setact("wizi",false)
    mob.level = ch.level + randnum(15,35) + (kills*5)

    mob:kill(ch)
    local ctrl=getmobworld(VM_CNTRL)[1]
    if ctrl then ctrl:say("shadowy darkness fighting %s", ch.name) end
end


function shadowquest.darkness_trigger(mob, ch, trigger, trigtype)
    if trigtype == "grall" then
        return darkness_grall(mob, ch, trigger)
    elseif trigtype == "death" then
        return darkness_death(mob, ch, trigger)
    end
end


local function crystal_put(obj, ch1, trigger)
    sendtochar(ch1, "The crystal cannot be put in containers.\n\r")
    return false
end

local function crystal_drop(obj, ch1, trigger)
    if ch1.room.area:flag("remort") then
        sendtochar(ch1, "Not in remort, BUB!\n\r")
        return
    end
    ch1.room:oload(VO_PORTAL)
    sendtochar(ch1, "As you drop the crystal a dark portal appears in front of you.\n\r")
    echoaround(ch1, "As %s drops a black crystal a dark portal appers in front of %s.", ch1.name, ch1.himher)

    if obj then obj:destroy() end
    return false
end


local function crystal_give(obj, ch1, trigger)
    sendtochar(ch1, "The crystal cannot be given to others.\n\r")
    return false
end

function shadowquest.crystal_trigger(obj, ch1, trigger, trigtype)
    if trigtype == "drop" then
        return crystal_drop(obj, ch1, trigger)
    elseif trigtype == "give" then
        return crystal_give(obj, ch1, trigger)
    elseif trigtype == "put" then
        return crystal_put(obj, ch1, trigger)
    end
end

local function sphere_drop(obj, ch1, trigger)
    if ch1.room.area:flag("remort") then
        sendtochar(ch1, "Not in remort, BUB!\n\r")
        return
    end

    -- if somebody is fighting, don't do it
    if #getroom(4703).players > 0 then
        sendtochar(ch1, "The sphere will not leave your hand...you had better try again later.\n\r")
        return false
    end


    ch1.room:oload(VO_PIT_PORTAL)
    sendtochar(ch1, "As you drop the sphere an abyssal pit opens just in front of your feet.\n\r")
    echoaround(ch1, "As %s drops a an obsidian sphere an abyssal pit opens just in front of %s feet.", ch1.name, ch1.hisher)

    if obj then obj:destroy() end
    return false
end

local function sphere_give(obj, ch1, trigger)
    sendtochar(ch1, "The sphere cannot be given to others.\n\r")
    return false
end

local function sphere_put(obj, ch1, trigger)
    sendtochar(ch1, "The sphere cannot be put in containers.\n\r")
    return false
end

function shadowquest.sphere_trigger(obj, ch1, trigger, trigtype)
    if trigtype == "drop" then
        return sphere_drop(obj, ch1, trigger)
    elseif trigtype == "give" then
        return sphere_give(obj, ch1, trigger)
    elseif trigtype == "put" then
        return sphere_put(obj, ch1, trigger)
    end
end

local function priest_grall(mob, ch, trigger)
     --[[ kill other pits
    for k,v in pairs(getobjworld(portal2)) do
        v.room:echo("The abyssal pit is suddenly gone, as if it never existed.")
        v:destroy()
    end
    --]]
    mob:setact("wizi",false)
    mob:say("Ah, so you have arrived.")
end

local function sweep_players(mob)
    for k,v in pairs(mob.room.players) do
        sendtochar(v,"You are suddenly lifted out of the abyss.\n\r")
        v:goto(10204)
        echoaround(v,"A pit opens momentarily from below and %s emerges.", ( v.ispc and v.name or v.shortdescr) )
    end
    
    for k,v in pairs(mob.room.mobs) do
        if not(mob == v) then v:destroy() end
    end
end

local groupfight
local round
local function priest_timer(mob, ch, trigger)
    if groupfight then -- fight has started, let's see if it finished
        local stillfighting = false
        for k,v in pairs(mob.room.people) do
            if v.position == "fight" then
                stillfighting = true
            end
        end
        
        if stillfighting then return end
        -- If we're here, the fight ended.
        groupfight = false
        -- see if any players survived
        if #mob.room.players < 1 then -- players died, reset
            for k,v in pairs(mob.room.people) do
                if not(v == mob) then v:destroy() end
            end
            return
        end
       
        -- players survived, give them cool stuff 
        local tm = os.time()
        for k,v in pairs(mob.room.players) do
            sendtochar(v,"You are suddenly lifted out of the abyss.\n\r")
            v:goto(10204)
            v:echoaround(v,"A pit opens momentarily from below and %s emerges.", ( v.ispc and v.name or v.shortdescr) )
            v:mdo("look")

            v:oload(VO_SCROLL)
            sendtochar(v,"You find a scroll made of charred skin in your inventory.\n\r")
            incr_group_kills(v)
            set_group_timer(v, tm + DLY_GROUP_FIGHT)
         end

         --cleanup just in case
         for k,v in pairs(mob.room.mobs) do
             if not(v == mob) then v:destroy() end
         end

         return
    end
  
    if #mob.room.players < 1 then return end      
    local ppl = mob.room.people
    if not(#ppl == 3) then -- mob + 2 players
        mob:say("I've told you to come as exactly 2!")
        mob:say("Be gone!")
        sweep_players(mob)
        return
    end

    -- start de fight
    local m1 = mob.room:mload(VM_COLOSSUS)
    local m2 = mob.room:mload(VM_SMOKE)
    round = 0

    -- SET THE LEVEL
    local kills = 0
    local sweep = false
    local tm = os.time()
    for k,v in pairs(mob.room.players) do
        local entry = get_player_entry(v)
        if entry and entry.group_kills >= MAX_GROUP_KILL then
            mob:say("%s, I don't want to see you again!", v.name)
            sweep = true
        elseif entry and entry.group_timer > tm then
            mob:say("%s, you must wait longer to return!", v.name)
            sweep = true
        elseif entry and entry.group_kills > kills then
            kills = entry.group_kills
        end
    end
    if sweep == true then
        --mdo("at vodur say sweepin")
        sweep_players(mob)
        return
    end

    local lv = math.max(mob.room.players[1].level, mob.room.players[2].level)
    m1.level = math.min(( ((lv+(kills*10))*(100 + randnum(30, 50)))/100),200)
    m2.level = math.min(( ((lv+(kills*10))*(100 + randnum(30, 50)))/100),200)
    --m1:mdo("tell vodur %d", m1.level)
    --m2:mdo("tell vodur %d", m2.level)

    local tgt1 = mob.room.players[1]
    local tgt2 = mob.room.players[2]

    if tgt1 then m1:kill(tgt1) end
    if tgt2 then m2:kill(tgt2) end

     -- kill other pits
    for k,v in pairs(getobjworld(VO_PIT_PORTAL)) do
        v.room:echo("The abyssal pit is suddenly gone, as if it never existed.")
        v:destroy()
    end

    groupfight = true
end

local function group_player_death()
    local room = getroom(VR_GROUP_FIGHT)

    -- fires during death before the dead char actually leaves room
    if #room.players < 2 then
        groupfight = nil
        round = nil

        for _,mob in pairs(room.mobs) do
            if not(mob.vnum == VM_PRIEST) then
                mob:destroy()
            end
        end
    end
end

function shadowquest.priest_trigger(mob, ch, trigger, trigtype)
    if trigtype == "grall" then
        return priest_grall(mob, ch, trigger)
    elseif trigtype == "timer" then
        return priest_timer(mob, ch, trigger)
    end
end

function shadowquest.groupfight_fight(mob, ch, trigger)
    round = round or 0
    round = round + 1 
    
    for i=0,round,8 do
        if ch.level>100 then
            mob:say("hit")
        end
        mob:hit(ch.name)
    end
end

function shadowquest.area_death(area, ch1, trigger, trigtype)
    if ch1.room.vnum == VR_GROUP_FIGHT then
        group_player_death() 
    end
end

shared.shadowquest = shadowquest
