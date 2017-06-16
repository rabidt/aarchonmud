--local dbg = true
if shared.grraka_loaded then return end

local grraka={}

local rwd_tbl
local animals=
{
{animal_name="Aardvark",location="land" },
{animal_name="Albatross",location="air" },
{animal_name="Alligator",location="water" },
{animal_name="Alpaca",location="land" },
{animal_name="Ant",location="land" },
{animal_name="Anteater",location="land" },
{animal_name="Antelope",location="land" },
{animal_name="Ape",location="land" },
{animal_name="Armadillo",location="land" },
{animal_name="Baboon",location="land" },
{animal_name="Badger",location="land" },
{animal_name="Barracuda",location="water" },
{animal_name="Bat",location="air" },
{animal_name="Bear",location="land" },
{animal_name="Beaver",location="land" },
{animal_name="Bee",location="air" },
{animal_name="Bison",location="land" },
{animal_name="Boar",location="land" },
{animal_name="Buffalo",location="land" },
{animal_name="Butterfly",location="air" },
{animal_name="Camel",location="land" },
{animal_name="Caribou",location="land" },
{animal_name="Cat",location="land" },
{animal_name="Caterpillar",location="land" },
{animal_name="Cattle",location="land" },
{animal_name="Chamois",location="land" },
{animal_name="Cheetah",location="land" },
{animal_name="Chicken",location="land" },
{animal_name="Chimpanzee",location="land" },
{animal_name="Chinchilla",location="land" },
{animal_name="Chough",location="air" },
{animal_name="Clam",location="water" },
{animal_name="Cobra",location="land" },
{animal_name="Cockroach",location="land" },
{animal_name="Cod",location="water" },
{animal_name="Cormorant",location="air" },
{animal_name="Coyote",location="land" },
{animal_name="Crab",location="water" },
{animal_name="Crane",location="air" },
{animal_name="Crocodile",location="water" },
{animal_name="Crow",location="air" },
{animal_name="Curlew",location="air" },
{animal_name="Deer",location="land" },
{animal_name="Dinosaur",location="land" },
{animal_name="Dog",location="land" },
{animal_name="Dogfish",location="water" },
{animal_name="Dolphin",location="water" },
{animal_name="Donkey",location="land" },
{animal_name="Dotterel",location="air" },
{animal_name="Dove",location="air" },
{animal_name="Dragonfly",location="air" },
{animal_name="Duck",location="land" },
{animal_name="Dugong",location="water" },
{animal_name="Dunlin",location="air" },
{animal_name="Eagle",location="air" },
{animal_name="Echidna",location="land" },
{animal_name="Eel",location="water" },
{animal_name="Eland",location="land" },
{animal_name="Elephant",location="land" },
{animal_name="Elephant seal",location="water" },
{animal_name="Elk",location="land" },
{animal_name="Emu",location="land" },
{animal_name="Falcon",location="air" },
{animal_name="Ferret",location="land" },
{animal_name="Finch",location="air" },
{animal_name="Fish",location="water" },
{animal_name="Flamingo",location="land" },
{animal_name="Fly",location="air" },
{animal_name="Fox",location="land" },
{animal_name="Frog",location="land" },
{animal_name="Galago",location="land" },
{animal_name="Gaur",location="land" },
{animal_name="Gazelle",location="land" },
{animal_name="Gerbil",location="land" },
{animal_name="Giant Panda",location="land" },
{animal_name="Giraffe",location="land" },
{animal_name="Gnat",location="air" },
{animal_name="Gnu",location="land" },
{animal_name="Goat",location="land" },
{animal_name="Goose",location="land" },
{animal_name="Goldfinch",location="air" },
{animal_name="Goldfish",location="water" },
{animal_name="Gorilla",location="land" },
{animal_name="Goshawk",location="air" },
{animal_name="Grasshopper",location="land" },
{animal_name="Grouse",location="air" },
{animal_name="Guanaco",location="land" },
{animal_name="Guinea fowl",location="land" },
{animal_name="Guinea pig",location="land" },
{animal_name="Gull",location="air" },
{animal_name="Hamster",location="land" },
{animal_name="Hare",location="land" },
{animal_name="Hawk",location="air" },
{animal_name="Hedgehog",location="land" },
{animal_name="Heron",location="land" },
{animal_name="Herring",location="air" },
{animal_name="Hippopotamus",location="land" },
{animal_name="Hornet",location="air" },
{animal_name="Horse",location="land" },
{animal_name="Hummingbird",location="air" },
{animal_name="Hyena",location="land" },
{animal_name="Jackal",location="land" },
{animal_name="Jaguar",location="land" },
{animal_name="Jay",location="air" },
{animal_name="Jellyfish",location="water" },
{animal_name="Kangaroo",location="land" },
{animal_name="Koala",location="land" },
{animal_name="Komodo dragon",location="land" },
{animal_name="Kouprey",location="land" },
{animal_name="Kudu",location="land" },
{animal_name="Lapwing",location="air" },
{animal_name="Lark",location="air" },
{animal_name="Lemur",location="land" },
{animal_name="Leopard",location="land" },
{animal_name="Lion",location="land" },
{animal_name="Llama",location="land" },
{animal_name="Lobster",location="water" },
{animal_name="Locust",location="land" },
{animal_name="Loris",location="land" },
{animal_name="Louse",location="land" },
{animal_name="Lyrebird",location="air" },
{animal_name="Magpie",location="air" },
{animal_name="Mallard",location="land" },
{animal_name="Manatee",location="water" },
{animal_name="Marten",location="land" },
{animal_name="Meerkat",location="land" },
{animal_name="Mink",location="land" },
{animal_name="Mole",location="land" },
{animal_name="Monkey",location="land" },
{animal_name="Moose",location="land" },
{animal_name="Mouse",location="land" },
{animal_name="Mosquito",location="air" },
{animal_name="Mule",location="land" },
{animal_name="Narwhal",location="water" },
{animal_name="Newt",location="land" },
{animal_name="Nightingale",location="air" },
{animal_name="Octopus",location="water" },
{animal_name="Okapi",location="land" },
{animal_name="Opossum",location="land" },
{animal_name="Oryx",location="land" },
{animal_name="Ostrich",location="land" },
{animal_name="Otter",location="water" },
{animal_name="Owl",location="air" },
{animal_name="Ox",location="land" },
{animal_name="Oyster",location="water" },
{animal_name="Panther",location="land" },
{animal_name="Parrot",location="air" },
{animal_name="Partridge",location="air" },
{animal_name="Peafowl",location="air" },
{animal_name="Pelican",location="air" },
{animal_name="Penguin",location="land" },
{animal_name="Pheasant",location="air" },
{animal_name="Pig",location="land" },
{animal_name="Pigeon",location="air" },
{animal_name="Pony",location="land" },
{animal_name="Porcupine",location="land" },
{animal_name="Porpoise",location="water" },
{animal_name="Prairie Dog",location="land" },
{animal_name="Quail",location="air" },
{animal_name="Quelea",location="air" },
{animal_name="Rabbit",location="land" },
{animal_name="Raccoon",location="land" },
{animal_name="Rail",location="air" },
{animal_name="Ram",location="land" },
{animal_name="Rat",location="land" },
{animal_name="Raven",location="air" },
{animal_name="Red deer",location="land" },
{animal_name="Red panda",location="land" },
{animal_name="Reindeer",location="land" },
{animal_name="Rhinoceros",location="land" },
{animal_name="Rook",location="air" },
{animal_name="Ruff",location="air" },
{animal_name="Salamander",location="land" },
{animal_name="Salmon",location="water" },
{animal_name="Sand Dollar",location="water" },
{animal_name="Sandpiper",location="air" },
{animal_name="Sardine",location="water" },
{animal_name="Scorpion",location="land" },
{animal_name="Sea lion",location="water" },
{animal_name="Sea Urchin",location="water" },
{animal_name="Seahorse",location="water" },
{animal_name="Seal",location="water" },
{animal_name="Shark",location="water" },
{animal_name="Sheep",location="land" },
{animal_name="Shrew",location="land" },
{animal_name="Shrimp",location="water" },
{animal_name="Skunk",location="land" },
{animal_name="Snail",location="land" },
{animal_name="Snake",location="land" },
{animal_name="Spider",location="land" },
{animal_name="Squid",location="water" },
{animal_name="Squirrel",location="land" },
{animal_name="Starling",location="air" },
{animal_name="Stingray",location="water" },
{animal_name="Stinkbug",location="land" },
{animal_name="Stork",location="air" },
{animal_name="Swallow",location="air" },
{animal_name="Swan",location="land" },
{animal_name="Tapir",location="land" },
{animal_name="Tarsier",location="land" },
{animal_name="Termite",location="land" },
{animal_name="Tiger",location="land" },
{animal_name="Toad",location="land" },
{animal_name="Trout",location="water" },
{animal_name="Turkey",location="land" },
{animal_name="Turtle",location="land" },
{animal_name="Vicuna",location="land" },
{animal_name="Viper",location="land" },
{animal_name="Vulture",location="air" },
{animal_name="Wallaby",location="land" },
{animal_name="Walrus",location="water" },
{animal_name="Wasp",location="air" },
{animal_name="Water buffalo",location="land" },
{animal_name="Weasel",location="land" },
{animal_name="Whale",location="water" },
{animal_name="Wolf",location="land" },
{animal_name="Wolverine",location="land" },
{animal_name="Wombat",location="land" },
{animal_name="Woodcock",location="air" },
{animal_name="Woodpecker",location="air" },
{animal_name="Worm",location="land" },
{animal_name="Wren",location="air" },
{animal_name="Yak",location="land" },
{animal_name="Zebra",location="land" }
}

do
  local exists
  for cnt in db:urows([[SELECT count(*) FROM sqlite_master WHERE type='table' AND name = 'grraka_animals']])
  do
    exists = ( cnt > 0 )
  end
  
  if not(exists) then
    log("Creating and populating grraka tables in script db.")
    -- Need to create and populate table
    -- assume all 3 tables need to be created
    local res = db:exec([[
      BEGIN;
      CREATE TABLE grraka_animals(
                    animal_name TEXT,
                    location TEXT);]])
    assert(res == sqlite3.OK, db:errmsg())


    local st = db:prepare[[
      INSERT INTO grraka_animals(animal_name, location)
        VALUES (?,?)]]

    for _,entry in pairs(animals) do
        st:reset()
        st:bind_values(entry.animal_name, entry.location)
        assert(st:step() == sqlite3.DONE)
    end
    st:finalize()

    res = db:exec[[CREATE TABLE grraka_seen(
                    animal_name TEXT,
                    player_name TEXT)]]
    assert(res == sqlite3.OK)

    res = db:exec[[CREATE TABLE grraka_caught(
                    animal_name TEXT,
                    player_name TEXT)]]
    assert(res == sqlite3.OK)

    local st_seen = db:prepare([[
      INSERT INTO grraka_seen(animal_name, player_name)
        VALUES(?,?)]])
    local st_caught = db:prepare([[
      INSERT INTO grraka_caught(animal_name, player_name)
        VALUES(?,?)]])

    -- grab old data from the tracker file
    local tbl=getroom(4700):loadtbl("animal_track")
    for _,animal in pairs(tbl) do
      for player_name,_ in pairs(animal.seen) do
        st_seen:reset()
        st_seen:bind_values(animal.name, player_name)
        assert(st_seen:step() == sqlite3.DONE)
      end

      for player_name,_ in pairs(animal.caught) do
        st_caught:reset()
        st_caught:bind_values(animal.name, player_name)
        assert(st_caught:step() == sqlite3.DONE)
      end
    end
    st_seen:finalize()
    st_caught:finalize()

    assert(sqlite3.OK == db:exec[[END]])
                 
    log("Done.")
  end

   -- reload to local variable from database
  animals = {}
  for entry in db:nrows[[SELECT animal_name, location FROM grraka_animals]]
  do
    table.insert(animals, entry)
  end
end

local st_check_seen = assert(db:prepare([[
  SELECT count(*) as cnt
  FROM grraka_seen
  WHERE animal_name = ?
  AND
  player_name = ?]]))
local function check_seen( ch, animal)
  st_check_seen:reset()
  st_check_seen:bind_values(animal, ch.name)
  for cnt in st_check_seen:urows() do
    return tonumber(cnt) > 0
  end
end


local st_set_seen = assert(db:prepare([[
  INSERT INTO grraka_seen(animal_name, player_name)
  VALUES (?,?)]]))
assert(st_set_seen)
local function set_seen( ch, animal)
  st_set_seen:reset()
  st_set_seen:bind_values(animal, ch.name)
  assert(sqlite3.DONE == st_set_seen:step())
end

local st_check_caught = assert(db:prepare([[
  SELECT count(*) as cnt
  FROM grraka_caught
  WHERE 
  animal_name = ? 
  AND
  player_name = ?]]))
local function check_caught( ch, animal)
  st_check_caught:reset()
  st_check_caught:bind_values(animal, ch.name)
  for cnt in st_check_caught:urows() do
    return tonumber(cnt) > 0
  end
end

local st_set_caught = assert(db:prepare([[
  INSERT INTO grraka_caught(animal_name, player_name)
  VALUES (?,?)]]))
assert(st_set_caught)
local function set_caught( ch, animal)
  st_set_caught:reset()
  st_set_caught:bind_values(animal, ch.name)
  assert(sqlite3.DONE == st_set_caught:step())
end

local st_count_caught = assert(db:prepare([[
  SELECT count(*) as cnt
  FROM grraka_caught
  WHERE
  player_name = ?]]))
local function count_caught( ch )
  st_count_caught:reset()
  st_count_caught:bind_values(ch.name)
  for cnt in st_count_caught:urows() do
    return cnt
  end
end

local function find_tracker( ch )
    local match 
    for k,v in pairs(ch.inventory) do
        if v.vnum==4844 then
            if v:getval("char_name")==ch.name then
                return v,true
            elseif not v:getval("char_name") then
                match=v
            end
        end
    end
    
    if match then -- they have a tracker but it's not started 
        return match,false
    end

    return false
end
 
local function animal_greet(mob, ch)
    if rand(40) then
        mob:delay(2, function() 
                    mob:emote("scurries away too quickly to follow.") 
                    mob:destroy()
        end)
    end

    local track,started=find_tracker(ch)
    if not track then return end

    if not started then track:setval("char_name",ch.name,true) end
        
    if check_seen(ch, mob:getval("animal_name")) then return end
    set_seen(ch, mob:getval("animal_name"))

    sendtochar(ch,"You mark "..mob.shortdescr.." as seen in your animal tracker.\n\r")
end

local function animal_kill(mob, ch)
    mob:emote("scurries away before you can cause it any damage.")
    mob:goto(2)
    mob:destroy()
end

local function animal_tackle(mob,ch)
    local chance=0
    local net
    for k,v in pairs(ch.inventory) do
        if v.wearlocation=="hold" then
            if v.vnum==4847 then -- weak net
                chance=60
                net=v
            elseif v.vnum==4846 then -- strong net
                chance=80
                net=v
            elseif v.vnum==4845 then -- unbreakable net
                chance=100
                net=v
            end

            break
         end
    end

    if chance<1 then
        sendtochar(ch,"Since you have no net to hold it, "..mob.shortdescr.." escapes you easily and scurries away!\n\r")
        mob:destroy()
        return
    end

    if rand(chance) then -- successful catch
        sendtochar(ch,"You have succesfully captured "..mob.shortdescr.."!\n\r")
        local netted=ch:oload(4843) -- load netted animal
        netted.name="captured "..mob.name
        netted.shortdescr="a captured "..mob.name
        netted.description="A captured "..mob.name.." is here."
        --netted:setval("captured_animal", mob:getval("animal_index"), true )
        netted:setval("animal_name", mob:getval("animal_name"), true)

        net:destroy()
        mob:destroy()
        return
    else
        sendtochar(ch,"You catch "..mob.shortdescr.." but it breaks through your net and scurries away!\n\r")
        net:destroy()
        mob:destroy()
        return
    end
end

local function animal_timer(mob, ch, env)
    -- every 15 minutes there's a chance for mob to 'wander away'
    -- chance scales linearly to reach 100% after 8 hours (480 minutes)
    env.minute_count=env.minute_count or 0
    env.minute_count=env.minute_count + 1

    -- only chance to destroy every 15 mins
    if not( (env.minute_count % 15) == 0 ) then return end
    
    local max=480
    local roll=(env.minute_count * 100)/max

    if rand(roll) then
        mob:emote("scurries away without warning!")
        mob:destroy()
    end

end

function grraka.animal_trigg(mob, ch, trigtype, env)
    if trigtype=="grall" then 
        animal_greet(mob, ch)
    elseif trigtype=="kill" then
        animal_kill(mob, ch)
    elseif trigtype=="social" then
        animal_tackle(mob, ch)
    elseif trigtype=="timer" then
        animal_timer(mob, ch, env)
    end
end

local secs=
{
    field="land",
    forest="land",
    hills="land",
    mountain="land",
    desert="land",
    air="air",
    shallow="water",
    deep="water",
    underwater="water"
}

local function trainer_timer(mob)
    -- clear out da gold
    mob.gold=0

    -- do placement if needed

    local ans=getmobworld(4847)
    if #ans>99 then return end

    for i=#ans,99 do
        local ind
        local num=#animals
        while true do
            local found
            ind=randnum(1,num)
            for k,v in pairs(ans) do
                if v:getval("animal_name")==animals[ind].animal_name then 
                    found = true 
                    break
                end
            end
            if not found then break end
        end

        local room
        while true do
            room=getrandomroom()
            if secs[room.sector]==animals[ind].location then
                break
            end
        end

        local an=room:mload(4847)
        local single=animals[ind].animal_name:find("^[AEIOU]") and "an" or "a"
        an.name=animals[ind].animal_name
        an.shortdescr=single.." "..animals[ind].animal_name
        an.longdescr=single.." "..animals[ind].animal_name.." is here."
        an.description="It's "..single.." "..animals[ind].animal_name..", ok?"
        an.level=1
        an:setval("animal_name",animals[ind].animal_name)
        table.insert(ans, an)
    end
end

local function trainer_speech(mob, ch, trigger )
    if trigger=="hello" then
        mob:emote("purrs 'Hello therre my frrriend.'")
        mob:mdo("wave")
        mob:emote("purrs 'What would you like to know? Say any of the following: story, reward, instructions, services'")
    elseif trigger=="story" then
        mob:emote("purrs 'I am Grraka, the animal trrainerrr. I have come to this land in search of animals of all kind.'")
        mob:emote("purrs 'I will rrreward you grreatly if you help to capturre animals for me!'")
    elseif trigger=="instructions" then
        mob:emote("purrs 'Therre arrre 224 different animals that interest me.'")
        mob:emote("purrs 'Simply buy an animal trracking sheet from me and some nets to begin yourrr hunt.'")
        mob:emote("purrs 'When you have caught an animal, just brring it herrre and drrrop it at my feet.'")
    elseif trigger=="reward" then
        mob:emote("purrs 'These are the rrrewarrds you may expect when you collect the rrrequisite number of animals: '")
        for i,v in pairs(rwd_tbl) do
            mob:say(v.cnt.. " animals: "..v.description)
        end
    elseif trigger=="services" then
        mob:emote("purrs 'I can help you to brrrand your pet...for a prrrice. Say 'brand' forrr morre inforrrmation.")
    else
        local args={}
        for word in trigger:gmatch("%S+") do
            table.insert(args, word)
        end

        if args[1]=="brand" then
            if not(args[2]) then
                mob:emote("purrs 'If you have a pet, just 'say brand <keyword>' to add a keyworrd to it.")
                mob:emote("purrs 'It will cost you 10 quest points")
                return
            end

            if not(ch.questpoints>9) then
                mob:emote("purrs 'You don't have enough quest points.'")
                return
            end

            if not(ch.pet) then
                mob:emote("purrs 'But you don't have a pet...'")
                return
            end

            table.remove(args, 1)
            if #args>1 then
                mob:emote("purrs 'You may only add one keyworrrd at a time...'")
                return
            end

            ch.pet.name=ch.pet.name.." "..args[1] 
            ch.pet.description=ch.pet.description.."\n\rIt is branded with the word: "..args[1]
            mob:emote("brands "..ch.pet.shortdescr.." with the keyword '"..args[1].."'.")
            mob:reward(ch, "qp", -10)
            return
        end
    end

    if not ch.isimmort then return end

    local args={}
    for word in trigger:gmatch("%S+") do
        table.insert(args,word)
    end

    if args[1]=="list" then
        mob:say("where        - where are the animals")
        mob:say("set [number] - set your caught # for testing")
        mob:say("status       - list players and # caught")
        mob:say("status <name>- show status for given player")
        mob:say("purge        - purge existing animals")
        mob:say("reload       - reload script file")
        mob:say("drop         - drop all grraka tables")
        mob:say("times        - show mob minute counts")

    elseif args[1]=="times" then
        for k,v in pairs(getmobworld(4847)) do
            local m=mob
            v:loadfunction(function()
                m:say(tostring(_G.minute_count or 0))
            end)
        end
    elseif args[1]=="drop" then
        start_con_handler( ch.descriptor, function(ch)
            sendtochar(ch, "Are you sure you want to drop all the grraka tables from db?\n\r[Y] to continue.")
            local cmd=coroutine.yield()

            if not(cmd=="Y") then
                sendtochar(ch, "Cancelled.\n\r")
                return
            end
            
            db:exec[[
                DROP TABLE grraka_animals;
                DROP TABLE grraka_seen;
                DROP TABLE grraka_caught;]]
            

        end, ch)
    elseif args[1]=="reload" then
        shared.grraka_loaded = nil
        shared.grraka=nil
        mob:say("done")
    elseif args[1]=="where" then
        local out={}
        for k,v in pairs(getmobworld(4847)) do
            table.insert(out,
                string.format("%-20s [%6d] %s{x (%s){x",
                    v.shortdescr,
                    v.room.vnum,
                    util.format_color_string(v.room.name,15),
                    v.room.area.name
                )
            )
        end
        pagetochar(ch, table.concat(out,"\n\r").."\n\r")
    elseif args[1]=="set" then
        if not tonumber(args[2]) then return end

        db:exec[[BEGIN]]
        db:exec( ([[
            DELETE FROM grraka_caught
            WHERE player_name = '%s']]):format(ch.name) )
        for i=1,tonumber(args[2]) do
            set_caught( ch, animals[i].animal_name)
        end
        db:exec[[END]]

        mob:say("done")
    elseif args[1]=="status" then
        local result
        if args[2] then
          for name, cnt in db:urows(([[
                SELECT player_name, count(*) as cnt
                FROM grraka_caught
                WHERE player_name = '%s']]):format(args[2]) )
          do
            mob:say("%-20s %d", name, cnt)
          end
        else
          for name, cnt in db:urows([[
                SELECT player_name, count(*) as cnt
                FROM grraka_caught
                GROUP BY player_name
                ORDER BY cnt]])
          do
            mob:say("%-20s %d", name, cnt)
          end
        end
        
    elseif args[1]=="purge" then
        local ans=getmobworld(4847)
        for _,animal in pairs(ans) do
            animal:destroy()
        end
        mob:say("Done")
    end

end

function grraka.trainer_trigg(mob, ch, trigtype, trigger, obj )
    if trigtype=="timer" then
        trainer_timer(mob)
    elseif trigtype=="speech" then
        trainer_speech(mob, ch, trigger)
    end
end

local st_trk_look = assert(db:prepare[[
  SELECT an.animal_name, seen.seen, caught.caught FROM
  grraka_animals as an
  LEFT JOIN
  ( SELECT animal_name, 'true' as seen FROM
    grraka_seen
    WHERE
    player_name = ?
  ) as seen on seen.animal_name = an.animal_name
  LEFT JOIN
  ( SELECT animal_name, 'true' as caught FROM
    grraka_caught
    WHERE
    player_name = ? 
  ) as caught on caught.animal_name = an.animal_name

  ORDER BY an.animal_name]]) 

local function tracker_look(obj, ch )
    local nm=obj:getval("char_name")
    if not nm then
        sendtochar( ch, "The tracking sheet is empty! There are just blank lines.\n\r")
        return false
    end
    
    local columns={}
    --local numans=#result
    local numans = #animals
    local numrows=math.ceil(numans/4)
    local ccnt=0

    st_trk_look:reset()
    st_trk_look:bind_values(nm, nm)

    local i = 0
    for v in st_trk_look:nrows() do
        i = i + 1
        local row
        row=i%numrows
        if row==0 then row=numrows end
        local color=v.caught and "{W" or v.seen and "{D" or ""
        local name=(v.seen or v.caught) and v.animal_name or "---"
        if v.caught then ccnt=ccnt+1 end
        columns[row]=columns[row] or ""
        columns[row]=string.format("%s %-3d %s%-15s{x",
                columns[row],
                i,
                color,
                name)
    end

    pagetochar( ch, nm.."'s animal tracking sheet! ({Dseen{x, {Wcaught{x)\n\r"..
            table.concat(columns, "\n\r")..
            "\n\r"..
            "Caught: "..ccnt..
            "\n\r"
    )
end

function grraka.tracker_trigg(obj, ch, trigtype)
    if trigtype=="look" then
        tracker_look(obj, ch)
    end
end

rwd_tbl =
{
    {
        cnt=25,
        description="A magical item that allows you to escape from anywherrre.",
        func=function(trainer,ch)
            local obj=ch:oload(4850)
            sendtochar(ch,"You receive "..obj.shortdescr.."!\n\r")
            return 
        end
    },
    {
        cnt=50,
        description="A special scrrroll with the power of the gods.",
        func=function(trainer,ch)
            local obj=ch:oload(4750)
            sendtochar(ch,"You receive "..obj.shortdescr.."!\n\r")
            return
        end
    },
    {
        cnt=100,
        description="A special containerrr to trrransferr quest points.",
        func=function(trainer,ch)
            local obj=ch:oload(4848)
            sendtochar(ch,"You receive "..obj.shortdescr.."!\n\r")
            return
        end
    },
    {
        cnt=150,
        description="Two powerrrful star frragments that will make you invincible.",
        func=function(trainer,ch)
            local obj=ch:oload(4851)
            sendtochar(ch, "You receive "..obj.shortdescr.."!\n\r")
            ch:oload(4851)
            sendtochar(ch, "You receive "..obj.shortdescr.."!\n\r")
            return
        end
    },
    {
        cnt=200,
        description="Anotherr godly scrroll and starrr frragment.",
        func=function(trainer,ch)
            local obj=ch:oload(4750)
            sendtochar(ch,"You receive "..obj.shortdescr.."!\n\r")
            obj=ch:oload(4851)
            sendtochar(ch,"You receive "..obj.shortdescr.."!\n\r")
            return
        end
    },
    {
        cnt=224,
        description="A powerrful device that severrrely injurres even the greatest of foes.",
        func=function(trainer,ch)
            local obj=ch:oload(4849)
            sendtochar(ch,"You receive "..obj.shortdescr.."!\n\r")
        end
    }
}



local function netted_drop(obj, ch)
    local trainer
    for k,v in pairs(ch.room.mobs) do
        if v.vnum==4848 then -- Grraka
            trainer=v
            break
        end
    end
    if not trainer then return true end

    -- check if carrying a tracking sheet
    local track,started=find_tracker(ch)

    if not track then
        trainer:emote("purrs 'You can't turrrrn in animals without your trrracking sheet.'")
        return true
    end

    if not started then
        track:setval("char_name",ch.name,true)
    end
    
    -- check if already turned this one in 
    local animal_name
    if obj:getval("captured_animal") then
        -- old version. no guarantee index matches current table so let's just parse it
        animal_name = obj.shortdescr:match("captured (.+)")
    else
        animal_name = obj:getval("animal_name")
    end

    if check_caught(ch, animal_name) then
        trainer:emote("purrs 'But you have alrrready given me "..obj.shortdescr.."'")
        return true
    end

    set_caught(ch, animal_name)

    ch:echo("%s drops %s.", ch.name, obj.shortdescr)
    sendtochar(ch, "You drop "..obj.shortdescr..".\n\r")
    trainer:emote("takes the captured animal and purrs happily.")
    trainer:reward(ch,"qp",5)
    -- check for extra rewards
    local ccnt=count_caught(ch)

    for k,v in pairs(rwd_tbl) do
        if ccnt==v.cnt then
            v.func(trainer,ch)
        end
    end

    obj:destroy()
end

local function netted_look(obj, ch)
    sendtochar(ch,"It's an animal captured in a net! Simply drop it at the Grraka for a reward.\n\r")
end

function grraka.netted_trigg(obj, ch, trigtype, trigger )
    if trigtype=="drop" then
        return netted_drop(obj,ch)
    elseif trigtype=="look" then
        return netted_look(obj,ch)
    end
end

local function net_look(obj, ch)
    sendtochar(ch, obj.description.."\n\r")
    sendtochar(ch, "To use it, simply 'hold' it in your hands and 'tackle' your target.\n\r")
    return false
end

function grraka.net_trigg(obj, ch, trigtype, trigger )
    if trigtype=="look" then
        return net_look(obj, ch)
    end
end

shared.grraka=grraka
shared.grraka_loaded = true
