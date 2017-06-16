local cols=5
local rows=7
local basevnum=19250

-- check for daily reset every time the script is loaded
local daily_track=getroom(basevnum).area:loadtbl("daily_track") or {expire=0}
local tm=os.time()
if (tm > daily_track.expire) then
    local now=os.date("*t")
    daily_track={ 
        expire=os.time{
            year=now.year, 
            month=now.month, 
            day=now.day+1, 
            hour=0, 
            minute=0, 
            second=0
        } 
    }
end

local function save_daily_track()
    getroom(basevnum).area:savetbl("daily_track", daily_track)
end

local function getvnum( col, row)
    return basevnum-1 + (row-1)*cols + col
end

local function setexflag( dir, flg)
    if not(mob.room[dir]:flag(flg)) then
        olc(dir.." "..flg)
    end
end

function show_maze(ch)
    local out={}

    for row=1,rows do
        local second={}
        for col=1,cols do
            local room=getroom(getvnum(col,row))
            table.insert(out, "#")
            
            if room.east then
                table.insert(out, room.east:flag("dormant") and "  " or "--")
            end
            
            if room.south then
                table.insert(second, room.south:flag("dormant") and "   " or  "|  ")
            end
        end
        table.insert(out,"\n\r"..table.concat(second).."\n\r")
    end

    sendtochar(ch, table.concat(out))

    sendtochar(ch, [[
-------------------------------------------
#          room
- |        passage
{y&{x          enemy
{r@ {R{v%{x        portal
]])
end 

function dig_maze()
    for row=1,rows do
        for col=1,cols do
            mdo("redit create "..getvnum(col,row) )
        end
    end

    -- kill existing exits
    for i=basevnum,(basevnum-1+rows*cols) do
        mdo("goto "..i)
        if mob.room.vnum==i then
            mdo("redit")
            for _,exit in pairs(mob.room.exits) do
                olc( exit.." delete" )
            end
        end
    end


    for row=1,rows do
        for col=1,cols do
            local vnum=getvnum(col,row)
            mdo("goto "..vnum)
            if not(row==1) then -- do north stuff
                olc("north dig "..getvnum(col,row-1))
                setexflag( "north", "door")
                setexflag( "north", "dormant")
            end
            if not(row==rows) then -- do south stuff
                olc("south dig "..getvnum(col,row+1))
                setexflag( "south", "door")
                setexflag( "south", "dormant")
            end
            if not(col==1) then -- do west stuff
                olc("west dig "..getvnum(col-1,row))
                setexflag( "west", "door")
                setexflag( "west", "dormant")
            end
            if not(col==cols) then -- do east stuff
                olc("east dig "..getvnum(col+1,row))
                setexflag( "east", "door")
                setexflag( "east", "dormant")
            end

            -- set flags and junk
            if not(mob.room.sector=="underground") then
                olc("sector underground")
            end
            
            if not(mob.room:flag("dark")) then
                olc("room dark")
            end
            if not(mob.room:flag("no_teleport")) then
                olc("room no_teleport")
            end
            if not(mob.room:flag("no_recall")) then
                olc("room no_recall")
            end
            if not(mob.room:flag("indoors")) then
                olc("room indoors")
            end

            -- Set name and description
            if col==1 and row==1 then
                olc("name {GSTARTING ROOM{x")
            elseif col==cols and row==rows then
                olc("name {GENDING ROOM{x")
            else
                olc("name A room in the maze")
            end

            --olc("delrp 0")
            --olc("addrp 19250 look 100")
                
        end
    end
end  


local function revdir( dir )
    if dir == "north" then return "south"
    elseif dir == "south" then return "north"
    elseif dir == "east" then return "west"
    elseif dir == "west" then return "east"
    end
end

local maze_list={}
for i=basevnum,(basevnum-1+ rows*cols) do
    table.insert(maze_list, i)
end

function set_maze()
    generate_maze( basevnum, maze_list)
end

function generate_maze( startvnum, vnumlist )
    --Randomized Prim's algorithm http://en.wikipedia.org/wiki/Maze_generation_algorithm

    -- 1. Start with a grid full of walls.
    for _,vnum in pairs(vnumlist) do
        local room=getroom(vnum)
        for k,v in pairs(room.exits) do
            room[v]:setflag("door", true)
            room[v]:setflag("dormant", true)
        end
    end


    -- 2. Pick a cell, mark it as part of the maze. Add the walls of the cell to the wall list.
    local walls={}
    local maze={}

    for k,v in pairs(getroom(startvnum).exits) do
        table.insert(walls, { dir=v, exit=getroom(startvnum)[v]} )
    end
    maze[getroom(startvnum)]=true

    -- 3. While there are walls in the list:
    while #walls>0 do
        -- Pick a random wall from the list.
        local wallind=randnum(1,#walls)
        local wall=walls[wallind]
        table.remove(walls, wallind)

        -- If the cell on the opposite side isn't in the maze yet:        
        if not maze[wall.exit.toroom] then
            -- Make the wall a passage and mark the cell on the opposite side as part of the maze.
            wall.exit:setflag("dormant", false)
            wall.exit:setflag("door", false)
            wall.exit.toroom[revdir(wall.dir)]:setflag("dormant", false)
            wall.exit.toroom[revdir(wall.dir)]:setflag("door", false)
            maze[wall.exit.toroom]=true
            -- Add the neighboring walls of the cell to the wall list.
            for k,v in pairs(wall.exit.toroom.exits) do
                table.insert(walls, { dir=v, exit=wall.exit.toroom[v]} )
            end
        end
    end
end

local function clear_status()
    local area=getroom(basevnum).area
    local tbl=area:loadtbl("maze_track") or {}

    tbl.current_player=nil
    tbl.start_time=nil
    area:savetbl("maze_track",tbl)
end

function try_start_maze( ch )
    local area=getroom(basevnum).area
    local tbl=area:loadtbl("maze_track") or {}
    
    if tbl.current_player then -- somebody in there
        sendtochar( ch, "Sorry, somebody's in there right now!\n\r")
        return
    end

    local ent=tbl[ch.name]
    local time_current=gettime() 

    if daily_track[ch.name] then
        sendtochar( ch, "Sorry, you already did the maze today! Come back tomorrow.\n\r")
        return
    end

    -- gen the maze
    set_maze()

    -- start the maze
    tbl.current_player=ch.name
    tbl.start_time=gettime()
    area:savetbl( "maze_track", tbl)

    ch:emote("enters the MAZE CHALLENGE!")
    ch:goto(basevnum)
    sendtochar( ch, "You have 1 minute to complete the maze!\n\r")
    area:delay( 60, function()
        if not(ch.valid) then
            return
        end
        sendtochar( ch, "You didn't complete the maze in time!!!\n\r")
        ch:goto(10204)
        --ch:emote("appears in the room!")
        echoaround(ch, ch.name.." appears in the room!")
        clear_status()
    end, "countdown")
end

function finish_maze( ch )
    local area=getroom(basevnum).area
    local tbl=area:loadtbl("maze_track") or {}
    if not(ch.name==tbl.current_player) then return end
    ch:goto(10204)
    ch:emote("appears in the room!")
    area:cancel("countdown")

    local time=gettime() - tbl.start_time
    sendtochar( ch, "You made it in "..time.." seconds!!!\n\r")
    ch:reward(ch, "qp", 15)
    tbl[ch.name]=tbl[ch.name] or {}
    local ent=tbl[ch.name]
    if time<(ent.best_time or math.huge) then
        sendtochar(ch, "That's a new best time for you!\n\r")
        ent.best_time=time
    end

    area:savetbl( "maze_track", tbl)

    daily_track[ch.name]=true
    save_daily_track()

    clear_status()

    -- do leaderboard stuff
    if not(ch.isimmort) then
        local lb=area:loadtbl( "maze_lb" ) or {}
        local ind
        local threshold=#lb<10 and math.huge or lb[#lb].score
        if threshold<=time then return end
        -- else they made it on the board, find out where
        ind = #lb+1 -- default is end of the table
        for i,v in ipairs(lb) do
            if time<v.score then
                ind=i
                break
            end
        end

        table.insert(lb, ind, {name=ch.name, score=time})
        if #lb>10 then
            table.remove(lb,11)
        end

        area:savetbl("maze_lb", lb)
    end
end

function area_trigger( ch1, trigger, trigtype)
    if trigtype=="quit" then
        local tbl=loadtbl("maze_track") or {}
        if tbl.current_player==ch1.name then
            ch1:goto(10204)
            sendtochar(ch1, "Quitting so soon?!?!\n\r")
            area:cancel("countdown")
            clear_status()
        end
    elseif trigtype=="death" then
        local tbl=loadtbl("maze_track") or {}
        if tbl.current_player==ch1.name then
            area:cancel("countdown")
            clear_status()
        end
    elseif trigtype=="boot" or trigtype=="connect" then
        -- clean up stuff from copyover or crash or whatev
        local plrs=area.players
        if #plrs>0 then
            echo("{WSORRY, IT SEEMS THERE WAS SOME ISSUE, PLEASE TRY AGAIN{x")
        end

        for k,v in pairs(plrs) do
            v:goto(10204)
        end

        clear_status()
    end
end

function room_trigger( ch1, ch2, obj1, obj2, text1, trigger, trigtype)
    if trigtype=="enter" then
        finish_maze(ch1)
    end
end

local function show_maze_usage( ch )
    sendtochar( ch, [[
quest maze enter  -- Enter the maze challenge!
quest maze status -- See info like who's in there and when you can go in!
quest maze time   -- Show your best time and top 10 overall best times!
]])
end
function quest_room_trigger( ch1, ch2, obj1, obj2, text1, trigger, trigtype)
    if text1 == "enter" then
        try_start_maze(ch1)
        return
    elseif text1 == "status" then
        -- print some junk here
        local tbl=getroom(basevnum):loadtbl("maze_track") or {}
        local plr = tbl.current_player and tbl.current_player or "NOBODY"
        sendtochar( ch1, "Currently in the maze: " .. plr .. "\n\r")

        if daily_track[ch1.name] then
            sendtochar(ch1, "You've already done the maze today! Try again tomorrow.\n\r")
        else
            sendtochar(ch1, "You haven't done the maze today! Feel free to enter.\n\r")
        end

    elseif text1 == "time" then
        -- print leaders
        sendtochar(ch1, "---------MAZE CHALLENGE LEADERS---------\n\r")
        local lb=getroom(basevnum):loadtbl("maze_lb") or {}
        for i,v in ipairs(lb) do
            sendtochar( ch1,
                string.format("%3d. %-20s %.2f seconds\n\r", i, v.name, v.score))
        end

        local tbl=getroom(basevnum):loadtbl("maze_track") or {}

        if not tbl[ch1.name] then
            sendtochar(ch1, "\n\rYou don't have a best time!\n\r")
        else
            sendtochar(ch1, ("\n\rYour best time: %.2f seconds.\n\r"):format(
                tbl[ch1.name].best_time) )
        end
    else
        show_maze_usage( ch1 )
    end

    return false
end
