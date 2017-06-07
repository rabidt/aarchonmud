--[[   
        Global Mob Quest. Written by Astark, credits to Vodur for help/debug/guidance.

    This script is used to control all aspects of the global mob quest. The quest is handled
    in game, room 4707. There is a controller mob (4703) and a prototype mob (4700) that are
    used. Using Lua, a mob is generated with a randomized name, and placed in a randomized
    location that is accessible to players. The mob's strength is based on the number of heroes
    online, and it uses a randomly selected, pre-scripted fight sequence during combat. Minions
    vnum (4701) are summoned to help the boss, the number of minions also based on number of
    heroes. To manually start a quest, goto room 4703, and type : "poke global".

--]]



-- Set this value to 1 if you want some debug/echo info
debug = false

-- Places the mob in a room that heroes can reach
local function area_place()
  while 1 do
  mobvnum = getmobproto(randnum(50, 32000))
    if mobvnum then
      mobroom = getroom(mobvnum.vnum)
      if mobroom and mobvnum.level>100
        and mobroom.area.ingame
        and mobroom.area.minlevel>=65
        and not(mobroom.area:flag("solo"))
        and not(mobroom.area:flag("noquest"))
        and not(mobroom.area:flag("remort"))
        and not(mobroom:flag("solitary")
          or mobroom:flag("private")
          or mobroom:flag("jail")
          or mobroom:flag("no_quest")
          or mobroom:flag("box_room")
          or mobroom:flag("arena")
          or mobroom:flag("nowhere")
          or mobroom:flag("newbies_only")
          or mobroom:flag("heroes_only")
          or mobroom:flag("gods_only")
          or mobroom:flag("imp_only")
          or mobroom:flag("safe")
          or mobvnum:affected("charm")
          or mobvnum:affected("astral")
          or mobvnum:act("safe")
          or mobvnum:act("train")
          or mobvnum:act("practice")
          or mobvnum:act("healer")
          or mobvnum:act("pet")
          or mobvnum:act("no_quest"))
        then break
      end
    end
  end

  gecho("  {WA new hero quest has been started!{x")
  gecho("+-------------------------------------------------+")
  gecho("  Mob  : %s", target.shortdescr)
  gecho("  Area : %s", mobroom.area.name)
  echo("  Room : %s [%-5d] %s", mobroom.name, mobroom.vnum, target.attacktype)
  gecho("+-------------------------------------------------+")

  target:goto(mobroom.vnum)

end


-- Table of name prefixes for semi-random mob names
local target_prefixes=
{
"Aglan",
"Ankar",
"Arnola",
"Batus",
"Budoc",
"Brannis",
"Brutus",
"Camlaur",
"Cenvil",
"Colley",
"Daravan",
"Deric",
"Dolovar",
"Duraskar",
"Enjukan",
"Eolybar",
"Ermita",
"Falord",
"Farad",
"Fengor",
"Fulgor",
"Gasher",
"Geserek",
"Gantur",
"Hakia",
"Hazak",
"Hunvar",
"Ibasan",
"Ikiris",
"Ingalad",
"Irrel",
"Jennon",
"Jerowyn",
"Jin",
"Katerk",
"Kevir",
"Kulvar",
"Larival",
"Lennikar",
"Lilyn",
"Loriwell",
"Lynessa",
"Manacar",
"Mearah",
"Muriel",
"Nealum",
"Nozkil",
"Nurigo",
"Panzare",
"Pothi",
"Pulsora",
"Ramera",
"Rangar",
"Rygar",
"Silidra",
"Simen",
"Surgey",
"Thorack",
"Tilzar",
"Trist",
"Unvek",
"Uragi",
"Ustes",
"Warax",
"Wervin",
"Wullsa",
"Zarot",
"Zeng",
"Zolfogar",
"Zimaf"
}



-- Table of name suffixes for semi-random mob names
local target_suffixes=
{
" Brambleclaw",
" Corpsemaker",
" Deathmaw",
" Doomfist",
" Dreadmaul",
" Dreamslayer",
" Filthmonger",
" Fleshsearer",
" Foulmist",
" Gloomflayer",
" Griefbringer",
" Howlspawn",
" Manglehound",
" of the Culled",
" of the Damned",
" of the Inferno",
" of the Void",
" Perilbringer",
" Pusblister",
" Rotbreath",
" Scourgeskin",
" Shacklegrasp",
" Siegehand",
" Soulsmiter",
" Steelfang",
" Stoneskin",
" Stonetongue",
" Stormbringer",
" the Banished",
" the Blighted",
" the Bone Crusher",
" the Defiler",
" the Destroyer",
" the Devourer",
" the Ethereal",
" the Forsaken",
" the Immortal",
" the Infiltrator",
" the Mind Eater",
" the Nefarious",
" the Returned",
" the Unholy",
" the Vile",
" the Violator",
" the Warlord",
" the Wretched",
" Thorntongue",
" Vipersong",
", Devastator of Souls",
", Herald of Doom",
", Master of Death",
", Templar of Pain",
", Viceroy of Destruction",
", Warden of Torment"
}



-- Table of color codes for mob name (no low green, blue, or white)
local target_color=
{
"{r",
"{c",
"{m",
"{y",
"{D",
"{R",
"{G",
"{C",
"{B",
"{M",
"{Y"
}



-- Table of races. The vulns, etc. get set automatically based on race.
race_table=
{
"ahazu",
"android",
"archon",
"behemoth",
"chrysalies",
"cyborg",
"djinn",
"doppelganger",
"dragonborn",
"dryad",
"ettin",
"frost-giant",
"gargoyle",
"genie",
"gholam",
"gorgon",
"harpy",
"illithid",
"lich",
"lillend",
"mermaid",
"minotaur",
"mummy",
"naga",
"naiad",
"phantom",
"quickling",
"rakshasa",
"revenant",
"tengu",
"titan",
"vampire",
"voadkin",
"warforged",
"werewolf",
"wisp",
"wraith"
}



-- Table for attacks used in fight_warr function
warr_attack=
{
{cmd="headbutt", lag=0},
{cmd="fatal",    lag=2}, 
{cmd="puncture", lag=0},
{cmd="spit",     lag=0},
{cmd="net",      lag=0},
{cmd="disarm",   lag=0},
{cmd="uppercut", lag=2},
{cmd="bash",     lag=2}
}



-- Table for attacks used in fight_mage function
mage_attack=
{
{cmd="cast 'lightning breath'", lag=0},
{cmd="cast 'fire breath'",      lag=0},
{cmd="cast 'acid breath'",      lag=0},
{cmd="cast 'frost breath'",     lag=0},
{cmd="cast 'dispel magic'",     lag=1},
{cmd="cast 'blind'",            lag=1},
{cmd="cast 'weaken'",           lag=1},
{cmd="cast 'stop'",             lag=1},
{cmd="cast 'energy drain'",     lag=1},
{cmd="cast 'unearth'",          lag=2}
}


-- Table for attacks used in fight_pala function
pala_attack=
{
{cmd="cast smote",  lag=2},
{cmd="swing",       lag=1},
{cmd="puncture",    lag=0},
{cmd="disarm",      lag=1},
{cmd="cast rimbol", lag=2}
}



-- Table for attacks used in fight_ninj function
ninj_attack=
{
{cmd="cast confusion", lag=1},
{cmd="circle",         lag=2},
{cmd="choke",          lag=0},
{cmd="trip",           lag=1},
{cmd="paroxysm",       lag=2},
{cmd="cast feeble",    lag=1}
}


-- Table for attacks used in fight_monk function
monk_attack=
{
{cmd="uppercut",            lag=2},
{cmd="cast heal self",      lag=1},
{cmd="cast 'dispel magic'", lag=1},
{cmd="trip",                lag=1},
{cmd="cast slow",           lag=1}
}


-- Table for attacks used in fight_necr function
necr_attack=
{
{cmd="cast necrosis",      lag=1},
{cmd="cast decompose",     lag=1},
{cmd="cast 'iron maiden'", lag=1},
{cmd="cast cone",          lag=0},
{cmd="cast 'tomb stench'", lag=0},
{cmd="cast zombie",        lag=0},
{cmd="cast blind",         lag=1},
{cmd="cast slow",          lag=1},
{cmd="cast weaken",        lag=1}
}



-- Table for damtype randomization
damtype_table=
{
  "slice",     -- slash
  "stab",      -- pierce
  "pound",     -- bash
  "digestion", -- acid
  "wrath",     -- energy
  "divine",    -- holy
  "shbite",    -- lightning
  "frbite",    -- cold
  "flbite",    -- fire
  "drain",     -- negative
  "txbite",    -- poison
  "illum",     -- light
  "anguish",   -- mental
  "soaking",   -- drowning
  "betrayal",  -- charm
  "sonic"      -- sound
}



-- Table for auras that can be cast
local auras=
{
  "absolute",
  "electrocution",
  "epidemic",
  "immolation",
  "quirky"
}



-- Table for fight functions
fight_table=
{
  "warr", 
  "mage",
  "pala",
  "ninj",
  "monk",
  "necr"
}



-- Helper function for recasting a random aura
local function aura_cast()
  mdo("cast "..auras[randnum(1,#auras)])
end



-- Helper function for re-sanc and re-hasting
local function rebuff()
  -- 18% chance to re-sanc, ignores being lagged.
  if not((mob:affected("sanctuary"))) then
    if rand(18) then
      emote("eats a pill of sanctuary.")
      cast("sanctuary self")
    end
  end

  -- 18% chance to re-haste, ignores being lagged.
  if not((mob:affected("haste"))) then
    if rand(18) then
      emote("eats a pill of haste.")
      cast("haste self")
    end
  end
end



-- Helper function for calculating number of heroes online
local function get_number_heroes()
  local num = 0
  for _,plr in pairs(getplayerlist()) do
    if plr.level >= 95 and plr.level <= 100 then
      num = num+1
    end
  end
 return num
end



-- Helper function for recalculating mob strength
local function update_mob()
  glob_num_heroes = glob_num_heroes or 0
  local new_num_heroes = get_number_heroes()

  if new_num_heroes > glob_num_heroes then
    mob.level = 135 + ((new_num_heroes-1)*4)
  end

  -- These are percentage values. Will change as mob level changes.
  if fight_method == "warr" then
    mob.damroll = 125
    mob.hitroll = 125
  elseif fight_method == "mage" then
    mob.damroll = 85
    mob.hitroll = 85
  elseif fight_method == "pala" then
    mob.damroll = 120
    mob.hitroll = 130
  elseif fight_method == "ninj" then
    mob.damroll = 115
    mob.hitroll = 135
  elseif fight_method == "monk" then
    mob.damroll = 120
    mob.hitroll = 120
  elseif fight_method == "necr" then
    mob.damroll = 85
    mob.hitroll = 85
  end

  glob_num_heroes = get_number_heroes()
end



-- Helper function for loading minions
local function summon_minion()
  local number_heroes = get_number_heroes()
  local number_minions = math.floor(number_heroes/2)

  if #getmobworld(4701) < number_minions then
    local target = mload(4701)
    force("minion follow bossmob")
    force("bossmob group minion")
  end
end



-- Helper function for targeting
local function retarget()
  local retarget = true
  local tgt

  if debug then echo("retarget called") end
  if debug then echo(mob.name) end
  if main_target then
    tgt=getpc(main_target)
    if not tgt then
      retarget=true
    elseif not tgt.room==mob.room then
      retarget=true
    else
      retarget=false
    end
  end
  
  if not retarget then return end
  
  if debug then say( "retargetting") end
  
  tgt=mob:randchar()
  if debug then echo("Target = " ..tgt.name) end
  
  if not tgt then
  if debug then say("Nobody's here.") end
    return
  end
  
  main_target=tgt.name
  
  if debug then say("Now targetting " .. main_target) end
end



-- Helper function for selecting mob fight function
local function choose_fight_function()
  return fight_scripts[fight_table[randnum(1, #fight_table)]]
end



-- Help function for checking to see if the mob is blinded
local function blindcheck()
  if mob:affected("blindness") or
    mob:affected("spit") or
    mob:affected("dirt kicking") or
    mob:affected("fire breath") or
    mob:affected("gouge") then
      if rand(25) then
        mdo("cast 'cure blindness'")
        return
      else
        return
      end
  end
end



-- Function to load the mob and set all values
function load_target(ch)
  -- Vnum of the mob prototype
  target = mload(4700)

  -- Full name of the mob and its race are semi-randomized based on tables above
  local mob_color = target_color[randnum(1, #target_color)]  
  local mob_prefix = target_prefixes[randnum(1, #target_prefixes)]
  local mob_suffix = target_suffixes[randnum(1, #target_suffixes)]
  local mob_race = race_table[randnum(1, #race_table)]
  local mob_damtype = damtype_table[randnum(1, #damtype_table)]

  -- Get number of heroes online to determine mob's initial power
  local number_heroes = get_number_heroes()

  target.name = mob_prefix..mob_suffix.." bossmob"                   
  target.shortdescr = mob_color..mob_prefix..mob_suffix.."{x" 
  target.longdescr = mob_color..mob_prefix..mob_suffix.." is here.{x" 
  target.race = mob_race
  target.level = 130 + ((number_heroes-1)*4)
  target.attacktype = mob_damtype
  target:setimmune("charm")
  target:setimmune("summon")
  target:addaffect("affects","custom_affect",100,-1,"none", 0,"no_trace","no_trace")

     
  if debug then echo("Number of heroes : " .. number_heroes) end
  if debug then echo("Mob level : " .. target.level) end
  if debug then echo("Mob name : " .. target.name) end
  if debug then echo("Attacktype : ".. target.attacktype) end
  area_place()
end




-- Function for Warrior fight sequence
fight_scripts={}
function fight_scripts.warr()

  local extra_hits = 0
  local attack = warr_attack[randnum(1,#warr_attack)] -- Select random attack
  lag_count = lag_count or 0                    -- Counter to simulate lag
  spellup = spellup or 0

  -- Mob will update its power if more players have logged on
  update_mob()

  -- Mob spells itself up once (for now)
  if spellup == 0 then
    cast("armor self")
    cast("shield self")
    cast("stone")
    spellup = 1
  end

  blindcheck()
  retarget()

  -- 20% chance each round to guard random player in room.
  if rand(20) then
    local r = mob:randchar() -- grab random player in room
    if r then
      mdo("guard "..r.name)
    end
  end

  -- Calls helper function with 33% chance to re-haste and re-sanc
  rebuff()

  -- 33% chance to restance into armed or unarmed stance
  if mob.stance == "default" then
    if rand(33) then
      mdo("stand")
      if mob:wears(46) then
        mdo("stance blade")
      else
        mdo("stance lion")
      end
    end
  end

  -- Count players in the room to determine number of extra attacks
  for k,v in pairs(mob.room.people) do
    if v.ispc then
      extra_hits = extra_hits + 1.5
    end
    if v.isnpc and v:affected("charm") then
      extra_hits = extra_hits + 1
    end
  end

  -- Execute extra attacks for additional players
  while extra_hits >= 1 do
    hit()
    extra_hits = extra_hits - 1
  end

  -- If the mob isn't lagged from a previous skill, it will use a new attack   
  if lag_count > 0 then
    lag_count = lag_count - 1
  else
    mdo(attack.cmd)
    lag_count = attack.lag
  end
end




-- Function for Mage fight sequence
function fight_scripts.mage()

  local number_heroes = get_number_heroes() -- Calling our helper function
  local extra_hits = 0
  local attack = mage_attack[randnum(1,#mage_attack)] -- Select random attack
  lag_count = lag_count or 0                          -- Counter to simulate lag
  spellup = spellup or 0

  update_mob()

  if spellup == 0 then
    cast("armor self")
    cast("fade")
    cast("'protection magic'")
    cast("reflection")
    cast("shield self")
    cast("stone")
    aura_cast()
    spellup = 1
  end

  blindcheck()
  retarget()

  -- Calls helper function with 33% chance to re-haste and re-sanc
  rebuff()

  -- 33% chance to re-aura, ignores being lagged.
  if not((mob:affected("elemental_shield"))) then
    if rand(33) then
      aura_cast()
    end
  end


  -- 33% chance to restance into armed or unarmed stance
  if mob.stance == "default" then
    if rand(33) then
      mdo("stand")
      mdo("stance arcana") -- Works unarmed and armed
    end
  end

  -- Count players in the room to determine number of extra attacks
  for k,v in pairs(mob.room.people) do
    if v.ispc then
      extra_hits = extra_hits + .5
    end
  end

  -- Execute extra attacks for additional players
  while extra_hits >= 1 do
    local r = mob:randchar() -- grab random player in room
    if r then
      mdo(attack.cmd.." "..r.name)
      extra_hits = extra_hits-1
    else
      return
    end
  end

  -- If the mob isn't lagged from a previous skill, it will use a new attack   
  if lag_count > 0 then
    lag_count = lag_count - 1
  else
    mdo(attack.cmd)
    lag_count = attack.lag
  end
end




-- Function for Paladin fight sequence
function fight_scripts.pala()

  local extra_hits = 0
  local attack = pala_attack[randnum(1,#pala_attack)] -- Select random attack
  lag_count = lag_count or 0                    -- Counter to simulate lag
  spellup = spellup or 0

  -- Mob will update its power if more players have logged on
  update_mob()

  -- Mob spells itself up once (for now)
  if spellup == 0 then
    cast("armor self")
    cast("shield self")
    cast("stone")
    cast("frenzy")
    cast("prayer")
    cast("heroism")
    cast("quirky")
    cast("'damned blade'")
    spellup = 1
  end

  blindcheck()
  retarget()

  -- 20% chance each round to guard random player in room.
  if rand(20) then
    local r = mob:randchar() -- grab random player in room
    if r then
      mdo("guard "..r.name)
    end
  end

  -- Calls helper function with 18% chance to re-haste and re-sanc
  rebuff()

  -- This value gets set in the loop below, used for selecting a stance
  local targets_in_room = 0

  -- Count players in the room to determine number of extra attacks
  for k,v in pairs(mob.room.people) do
    targets_in_room = targets_in_room + 1
    if v.ispc then
      extra_hits = extra_hits + 1
    end
    if v.isnpc and v:affected("charm") then
      extra_hits = extra_hits + .5
    end
  end

  -- 40% chance to restance, will be based on targets_in_room and HP
  if mob.stance == "default" then
    if rand(40) then
      mdo("stand")
      if (mob.hp/mob.maxhp)*100 > 33 then
        if targets_in_room >= 4 then
          mdo("stance jihad")
        else
          mdo("stance blade")
        end
      else
        mdo("stance swayde")
      end
    end
  end


  -- Execute extra attacks for additional players
  while extra_hits >= 1 do
    hit()
    extra_hits = extra_hits - 1
  end

  -- If the mob isn't lagged from a previous skill, it will use a new attack   
  if lag_count > 0 then
    lag_count = lag_count - 1
  else
    mdo(attack.cmd)
    lag_count = attack.lag
  end
end




-- Function for Ninja fight sequence
function fight_scripts.ninj()

  local extra_hits = 0
  local attack = ninj_attack[randnum(1,#ninj_attack)] -- Select random attack
  lag_count = lag_count or 0                    -- Counter to simulate lag
  spellup = spellup or 0

  -- Mob will update its power if more players have logged on
  update_mob()

  -- Mob spells itself up once (for now)
  if spellup == 0 then
    cast("armor self")
    cast("shield self")
    cast("stone")
    mdo("tumbl")
    spellup = 1
  end

  blindcheck()
  retarget()

  -- Calls helper function with 18% chance to re-haste and re-sanc
  rebuff()

  -- 50% chance to restance into armed or unarmed stance
  if mob.stance == "default" then
    if rand(50) then
      mdo("stand")
      mdo("stance lion")
    end
  end

  -- Count players in the room to determine number of extra attacks
  for k,v in pairs(mob.room.people) do
    extra_hits = extra_hits + 1
  end

  -- Execute extra attacks for additional players
  while extra_hits >= 1 do
    hit()
    extra_hits = extra_hits - 1
  end

  -- If the mob isn't lagged from a previous skill, it will use a new attack   
  if lag_count > 0 then
    lag_count = lag_count - 1
  else
    mdo(attack.cmd)
    lag_count = attack.lag
  end

  -- 15% chance to distract a player each round when more than 1 player
  if #mob.room.players > 1 then
    if rand(15) then
      local r = mob:randchar() -- grab random player in room
      if r then
        mdo("distract "..r.name)
      end
    end
  end
end




-- Function for Monk fight sequence
function fight_scripts.monk()

  local extra_hits = 0
  local attack = monk_attack[randnum(1,#monk_attack)] -- Select random attack
  lag_count = lag_count or 0                    -- Counter to simulate lag
  spellup = spellup or 0

  -- We don't want monks using weapons!
  mdo("remove simple")
  mob:junk("simple")

  -- Mob will update its power if more players have logged on
  update_mob()

  -- Mob spells itself up once (for now)
  if spellup == 0 then
    cast("armor self")
    cast("shield self")
    cast("stone")
    cast("prayer")
    cast("frenzy")
    cast("fly")
    spellup = 1
  end

  blindcheck()
  retarget()

  -- 15% chance each round to guard or choke random player in room.
  if rand(15) then
    local r = mob:randchar() -- grab random player in room
    if r then
      if r.class == "cleric" or
      r.class == "mage" or
      r.class == "templar" or
      r.class == "illusionist" or
      r.class == "necromancer" then
        mdo("choke "..r.name)
      else
        mdo("guard "..r.name)
      end
    end
  end

  -- Calls helper function with 18% chance to re-haste and re-sanc
  rebuff()

  -- 33% chance to restance each round
  if mob.stance == "default" then
    if rand(33) then
      mdo("stand")
      if (mob.hp/mob.maxhp)*100 > 33 then
        if rand(50) then
          mdo("stance rhino")
        else
          mdo("stance lion")
        end
      else
        mdo("stance phoenix")
      end
    end
  end

  -- Monks have kung-fu, lets simulate it
  mdo("kick")
  mdo("chop")

  -- Count players in the room to determine number of extra attacks
  for k,v in pairs(mob.room.people) do
    extra_hits = extra_hits + 1
  end

  -- Execute extra attacks for additional players
  while extra_hits >= 1 do
    hit()
    extra_hits = extra_hits - 1
  end

  -- If the mob isn't lagged from a previous skill, it will use a new attack   
  if lag_count > 0 then
    lag_count = lag_count - 1
  else
    mdo(attack.cmd)
    lag_count = attack.lag
  end
end




-- Function for Necromancer fight sequence
function fight_scripts.necr()

  local number_heroes = get_number_heroes() -- Calling our helper function
  local extra_hits = 0
  local attack = necr_attack[randnum(1,#necr_attack)] -- Select random attack
  lag_count = lag_count or 0                          -- Counter to simulate lag
  spellup = spellup or 0

  update_mob()

  if spellup == 0 then
    cast("armor self")
    cast("fade")
    cast("'protection magic'")
    cast("reflection")
    cast("shield self")
    cast("stone")
    cast("epidemic")
    spellup = 1
  end

  blindcheck()
  retarget()

  -- Calls helper function with 18% chance to re-haste and re-sanc
  rebuff()

  -- 33% chance to restance into armed or unarmed stance
  if mob.stance == "default" then
    if rand(33) then
      mdo("stand")
      mdo("stance arcana") -- Works unarmed and armed
    end
  end

  -- Count players in the room to determine number of extra attacks
  for k,v in pairs(mob.room.people) do
    if v.ispc then
      extra_hits = extra_hits + .5
    end
  end

  -- Execute extra attacks for additional players
  while extra_hits >= 1 do
    local r = mob:randchar() -- grab random player in room
    if r then
      mdo(attack.cmd.." "..r.name)
      extra_hits = extra_hits - 1
    else
      return
    end
  end

  -- If the mob isn't lagged from a previous skill, it will use a new attack   
  if lag_count > 0 then
    lag_count = lag_count - 1
  else
    mdo(attack.cmd)
    lag_count = attack.lag
  end
end




-- Function that gets called in game to reference the above fight functions
function fight_script()
  fight_method = fight_method or choose_fight_function()
  fight_method()
  -- Removed minions for now
  -- summon_minion()
end



-- Fight script for minions
function minion_fight()
  local lag_count = lag_count or 0

  -- 33% chance to restance
  if mob.stance == "default" then
    if rand(33) then
      mdo("stand")
      mdo("stance shadowwalk") -- Works unarmed
    end
  end

  if lag_count > 0 then
    lag_count = lag_count - 1
  else
    if rand(25) then
      mdo("cast 'cure mental' bossmob")
      lag_count = lag_count + 2.5
    elseif rand(33) then
      mdo("cast 'cure mortal' bossmob")
      lag_count = lag_count + 2
    elseif rand(50) then
      mdo("cast 'major group heal'")
      lag_count = lag_count + 1.5
    else
      hit()
    end
  end
end













