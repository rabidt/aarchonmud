
if shared.shadowquest then return end
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
  place = {
    desc = "Force a shadow placement.",
    func = function(mob, ch, trigger)
      cntrl_place_shadows()
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
            string.format("%-20s |%5d %5dqp |%5d %5dqp |%5d\n\r", 
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

local VO_CRYSTAL = 4748 -- create portal to shadowy darkness
local VO_PORTAL = 4749 -- portal to shadowy darkness
local VO_SCROLL = 4750 -- scroll of charred skin
local VO_PIT_PORTAL = 4751 -- portal to pit
local VO_SPHERE = 4752 -- create pit portal

local DLY_PLACE_SHADOWS = 300 -- 5 mins
local DLY_CHECK_CRYSTAL = 360 -- 6 mins
local DLY_CHECK_SPHERE = 720 -- 12 mins


local function cntrl_check_crystals()
  local lst = getplayerlist()
  local currtime = os.time()

  for _,plr in pairs(lst) do
    local entry = get_player_entry(plr)

    if entry
      and entry.shadow_kills >= 20 
      and entry.darkness_timer < currtime
      and not(plr.room.area:flag("remort"))
      and not(plr:carries(VO_CRYSTAL))
    then
      plr:oload(VO_CRYSTAL)
      sendtochar(plr, "A black crystal appears in your inventory.\n\r")
      say("gave crystal to %s", plr.name)
    end
  end          
end

-- forward declared above
cntrl_place_shadows = function()
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
      and shadow_kills < 20
    then
      local room = plr.room
      local war = room:mload(VM_SHADOW)
      war.level = plr.level + (shadow_kills/2) + randnum(-5,5)
      if shadow_kills > 10 then
        war:kill(plr)
      end
      room:echo("%s appears seemingly from nowhere at the corner of your vision.", war.shortdescr)
      say("placed level %d in %d with  %s", war.level, room.vnum, plr.name)
    end
  end
end

local function cntrl_check_spheres()
  local lst = getplayerlist()
  local currtime = os.time()

  for _,plr in pairs(lst) do
    local entry = get_player_entry(plr)
    
    if entry 
      and entry.darkness_kills >= 10
      and entry.group_kills < 10
      and entry.group_timer < currtime
      and not(plr.room.area:flag("remort"))
      and not(plr:carries(VO_SPHERE))
    then
      plr:oload(VO_SPHERE)
      sendtochar(plr, "{DThe shadow priest speaks into your mind.\n\r{x")
      sendtochar(plr, "%s, you have had much conquest against my minions.\n\r", plr.name)
      sendtochar(plr, "When you are ready for a greater challenge, drop this globe and enter the abyss.\n\r")
      sendtochar(plr, "However, you must bring a partner. There must be 2, no more, no less.\n\r")
      say("gave sphere to %s", plr.name)
    end
  end
end

local function cntrl_timer(mob)
  local currtime = os.time()
  shad_time = shad_time or 0
  check_port_time = check_port_time or 0
  check_obstime = check_obstime or 0

  -- if (currtime > check_port_time) then
  --   mob:say("sending crystal")
  --   cntrl_check_crystals()
  --   mob:say("done")
  --   check_port_time = currtime + DLY_CHECK_CRYSTAL
  -- end

  if (currtime > shad_time) then
    mob:say("placing shadows")
    cntrl_place_shadows()
    mob:say("place done")
    shad_time = currtime + DLY_PLACE_SHADOWS
  end

  -- if (currtime > check_obstime) then
  --   say("sending spheres")
  --   cntrl_check_spheres()
  --   say("done")
  --   check_obstime = currtime + DLY_CHECK_SPHERE
  -- end
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

  local rwd = randnum(3,8)
  mob:reward(ch, "qp", rwd)

  incr_shadow_kills(ch)
  add_shadow_rwd(ch, rwd)
end

shared.shadowquest = shadowquest