local area = getroom(10204).area

local heart=[[
                     z$6*#""""*c     :@$$****$$$$L
                  .@$F          "N..$F         '*$$
                 /$F             '$P             '$$r
                d$"                                #$      '%C"""$
               4$F                                  $k    ud@$ JP
               M$                                   J$*Cz*#" Md"
               MR                              'dCcm#$       "
               MR                               )    $
               4$                                   4$
                $L                                  MF
                '$                                 4$
                 ?B .z@r                           $
               .+(2d"" ?                          $~
    +$c  .z4Cn*"   "$.                           $
'#*M3$Eb*""         '$c                         $
   /$$RR              #b                      .R
   6*"                 ^$L                   JF
                         "$                 $
                           "b             u"
                             "N.        xF
                               '*c    zF
                                  "N@"
]]

local goku=[[
                       _
                       \"-._ _.--"~~"--._
                        \   "            ^.    ___
                        /                  \.-~_.-~
                 .-----'     /\/"\ /~-._      /
                /  __      _/\-.__\L_.-/\     "-.
               /.-"  \    ( ` \_o>"<o_/  \  .--._\
              /'      \    \:     "     :/_/     "`
                      /  /\ "\    ~    /~"
                      \ I  \/]"-._ _.-"[
                   ___ \|___/ ./    l   \___   ___
              .--v~   "v` ( `-.__   __.-' ) ~v"   ~v--.
           .-[   |     :   \_    "~"    _/   :     |   ]-.
          /   \  |           ~-.,___,.-~           |  /   \
         ]     \ |                                 | /     [
         /\     \|     :                     :     |/     /\
        /  ^._  _K.___,^                     ^.___,K_  _.^  \
       /   /  "~/  "\                           /"  \~"  \   \
      /   /    /     \ _          :          _ /     \    \   \
    .^--./    /       Y___________l___________Y       \    \.--^.
    [    \   /        |        [/    ]        |        \   /    ]
    |     "v"         l________[____/]________j  -Row   ]r"     /
    ]------t          /                       \       /`-.     /
    |      |         Y                         Y     /    "-._/
    ]-----v'         |         :               |     7-.     /
    |   |_|          |         l               |    / . "-._/
    l  .[_]          :          \              :  r[]/_.  /
     \_____]                     "--.             "-.____/
]]

local timmay=[[
                       _.::::::::::::::::::::::::._
                    _J::::::::::::::::::::::::::::::-.
                 _,J::::;::::::!:::::::::::!:::::::::::-."\_ ___
              ,-:::::/::::::::::::/''''''-:/   \::::::::::::::::L_
            ,;;;;;::!::/         V               -::::::::::::::::7
          ,J:::::::/ \/                              '-'`\:::::::.7
          |:::::::'                                       \::!:::/
         J::::::/                                          `.!:\ dp
         |:::::7                                             |/\:\
        J::::/                                               \/ \:|
        |:::/                                                    \:\
        |::/                                                     |:.Y
        |::7                                                      |:|
        |:/                              `OOO8ooo._               |:|
        |/               ,,oooo8OO'           `"`Y8o,             |'
         |            ,odP"'                      `8P            /
         |          ,8P'    _.__         .---.._                /
         |           '   .-'    `-.    .'       `-.            /
         `.            ,'          `. /            `.          L_
       .-.J.          /              Y               \        /_ \
      |    \         /               |                Y      // `|
       \ '\ \       Y          8B    |   8B           |     /Y   '
        \  \ \      |                |                ;    / |  /
         \  \ \     |               ,'.              /    /  L |
          \  J \     \           _.'   `-._       _.'    /  _.'
           `.___,\    `._     _,'          '-...-'     /'`"'
                  \      '---'  _____________         /
                   `.           \|T T T T T|/       ,'
                     `.           \_|_|_|_/       .'
                       `.         `._.-..'      .'
                         `._         `-'     _,'
                            `--._________.--'
]]

local fivebyfive=[[
   
  1    2    3    4    5
   
   
  6    7    8    9   10
   
   
 11   12   13   14   15
   
   
 16   17   18   19   20
   
   
 21   22   23   24   25
   
]]

local function random_move( tiles )
    -- "move" the blank in a random dir

    -- find the empty tile
    local TILE_DIMENSION=tiles.dim
    local empty_y=tiles.empty_y
    local empty_x=tiles.empty_x

    local new_x=empty_x
    local new_y=empty_y
    local dirs={}
    if empty_x>1 then table.insert(dirs, "l") end
    if empty_x<TILE_DIMENSION then table.insert(dirs, "r") end
    if empty_y>1 then table.insert(dirs, "u") end
    if empty_y<TILE_DIMENSION then table.insert(dirs, "d") end

    local dir=dirs[randnum(1,#dirs)]
    if dir=="l" then
        new_x=new_x-1
    elseif dir=="r" then
        new_x=new_x+1
    elseif dir=="u" then
        new_y=new_y-1
    elseif dir=="d" then
        new_y=new_y+1
    end
   
    -- do the swap 
    tiles[empty_y][empty_x]=tiles[new_y][new_x]
    tiles[new_y][new_x]=nil
    tiles.empty_y=new_y
    tiles.empty_x=new_x
     
end

-- return a new layout
local function new_game(dim, pic)
    local game={}
    local orig={}
    local TILE_DIMENSION=dim
    game.orig=orig
    orig.dim=TILE_DIMENSION

    -- Make empty tile grid
    for y=1,TILE_DIMENSION do
        orig[y]={}
        for x=1,TILE_DIMENSION do
            orig[y][x]={}
        end
    end

    -- Populate tiles from the picture
    local lines={}
    local width=0
    local height=0
    for line in string.gmatch(pic, "[^\n]+") do
        -- trim the line
        local m=string.match(line,"^(.-)%s+$") or line
        table.insert(lines,m)
        if m:len()>width then width=m:len() end
    end

    -- pad the right with spaces
    width = width + (TILE_DIMENSION - width%TILE_DIMENSION)
    for k in pairs(lines) do
        lines[k]=string.format("%-"..width.."s", lines[k])
    end

    height = math.ceil(#lines/TILE_DIMENSION) * TILE_DIMENSION
    -- pad top and bottom with empty lines to make tiles uniform
    for i=(height-#lines),1,-1 do
        if i%2==0 then
            table.insert(lines, (" "):rep(width) ) -- bottom
        else
            table.insert(lines, 1, (" "):rep(width) ) -- top
        end
    end


    local tileheight=math.ceil(height/TILE_DIMENSION)
    local tilewidth=math.ceil(width/TILE_DIMENSION)

    for i,line in ipairs(lines) do
        local y=math.floor((i-1)/tileheight)+1
     
        for x=1,TILE_DIMENSION do
            table.insert(
                orig[y][x], 
                line:sub( 
                    (x-1)*tilewidth + 1, 
                    (x==TILE_DIMENSION) and nil or (tilewidth*x)))
        end
    end

    -- remove the bottom right corner tile
    orig[TILE_DIMENSION][TILE_DIMENSION]=nil
    orig.empty={y=TILE_DIMENSION, x=TILE_DIMENSION}

    -- make the actual game board
    local curr={}
    game.curr=curr
    curr.dim=TILE_DIMENSION

    -- first copy the completed puzzle
    for y=1,TILE_DIMENSION do
        curr[y]={}
        for x=1,TILE_DIMENSION do
            curr[y][x]=orig[y][x]
        end
    end
    curr.empty_y=TILE_DIMENSION
    curr.empty_x=TILE_DIMENSION

    -- now shuffle the heck out of it
    --[[
    for i=1,100 do
        random_move(curr)
    end
    --]]

    --[[ from bobble:
    numbers = [0..n]
    odd = false
    for (pos = 0..n-2)
    .   swap_pos = random(pos..n)
    .   if (pos != swap_pos)
    .       swap numbers[pos] and numbers[swap_pos]
    .       odd = !odd
    if (odd)
    .   swap numbers[n-1] and numbers[n]
    --]]

    local n=TILE_DIMENSION^2 - 3
    local odd=false
    for pos=1,n do
        local y=math.ceil(pos/TILE_DIMENSION)
        local x=TILE_DIMENSION - pos%TILE_DIMENSION

        local swap_pos=randnum(pos,n)
        if not(pos == swap_pos) then
            local swap_y=math.ceil(swap_pos/TILE_DIMENSION)
            local swap_x=TILE_DIMENSION - swap_pos%TILE_DIMENSION
            local tile=curr[y][x]
            local swap_tile=curr[swap_y][swap_x]

            curr[y][x]=swap_tile
            curr[swap_y][swap_x]=tile

            odd=not(odd)
         end

    end

    -- for laziness, assume dimension of at least 3, so last 2 tiles are definitely on the bottom row
    if odd then
        local tile=curr[TILE_DIMENSION][TILE_DIMENSION-2]
        local swap_tile=curr[TILE_DIMENSION][TILE_DIMENSION-1]

        curr[TILE_DIMENSION][TILE_DIMENSION-1]=tile
        curr[TILE_DIMENSION][TILE_DIMENSION-2]=swap_tile
     end


        



    return game
end


local function show_tiles( ch, tiles )
    local TILE_DIMENSION=tiles.dim
    local out={}
    for y=1,TILE_DIMENSION do
        -- ugly cause tiles[y][1] might be nil
        local tilewidth=(tiles[y][1] and tiles[y][1][1]:len() or tiles[y][2][1]:len())
        for linenum=1,(tiles[y][1] and #tiles[y][1] or #tiles[y][2]) do
            for x=1,TILE_DIMENSION do
                table.insert(out, 
                    tiles[y][x] and tiles[y][x][linenum] or (" "):rep(tilewidth))
                if x==TILE_DIMENSION then
                    table.insert(out, "\n\r")
                else
                    table.insert(out, "|")
                end
            end
        end
        if y<TILE_DIMENSION then
            table.insert(out, ("-"):rep(tilewidth*TILE_DIMENSION + TILE_DIMENSION).."\n\r")
        end
    end

    sendtochar( ch, table.concat(out))
             
end

-- show a mini grid to easily see where the empty space is 
local function show_grid( ch, tiles )
    local TILE_DIMENSION=tiles.dim
    for y=1,TILE_DIMENSION do
        sendtochar( ch, "|")
        for x=1,TILE_DIMENSION do
            sendtochar( ch, tiles[y][x] and "*|" or " |" )
        end
        sendtochar( ch, "\n\r")
    end
end

local function check_time(ch, size, start_time, end_time)
    local time = end_time - start_time
    sendtochar(ch, "You made it in "..time.." seconds!!!\n\r")

    local lb = area:loadtbl("slide_lb") or {}
    local leaders = lb[size]
    if leaders == nil then
        leaders = {}
        lb[size] = leaders
    end

    local ind
    local threshold = #leaders < 10 and math.huge or leaders[#leaders].score

    --ch:say("threshold:"..threshold)
    --ch:say("time:"..time)
    if threshold <= time then return end

    ind = #leaders + 1
    for i,v in ipairs(leaders) do
        if time < v.score then
            ind = i
            break
        end
    end

    table.insert(leaders, ind, {name=ch.name, score=time})
    if #leaders > 10 then
        table.remove(leaders, 11)
    end

    area:savetbl("slide_lb", lb)
end

local function game_con( ch )
    local TILE_DIMENSION 
    local pic

    while true do
        sendtochar( ch, [[
Select game mode:
    a) 3x3 [20qp reward]
    b) 4x4 [25qp reward]
    c) 5x5 [30qp reward]

]])
        local cmd=coroutine.yield()
        if cmd=="a" then
            TILE_DIMENSION=3
            break
        elseif cmd=="b" then
            TILE_DIMENSION=4
            break
        elseif cmd=="c" then
            TILE_DIMENSION=5
            break
        else
            sendtochar( ch, "Invalid response.\n\r")
        end
    end

    while true do
        if TILE_DIMENSION == 5 then
            pic=fivebyfive
            break
        end

        sendtochar( ch, [[
Select picture:
    a) Heart
    b) Goku
    c) Timmay

]])
        local cmd=coroutine.yield()
        if cmd=="a" then
            pic=heart
            break
        elseif cmd=="b" then
            pic=goku
            break
        elseif cmd=="c" then
            pic=timmay
            break
        else
            sendtochar( ch, "Invalid response.\n\r")
        end
    end

    
        
    local g=new_game(TILE_DIMENSION, pic)
    
    

    sendtochar( ch, "Completed:\n\r")
    show_tiles( ch, g.orig )
    sendtochar( ch, "[Hit return to continue]\n\r")

    coroutine.yield()

    local start_time = os.time()
    sendtochar( ch, "Let the game begin:\n\r")
    show_tiles( ch, g.curr )
    show_grid( ch, g.curr )

    while true do
        sendtochar( ch, "Enter direction (or 'help').\n\r")
        local cmd=coroutine.yield()

        if cmd=="help" then
            sendtochar( ch, [[
To move a tile, enter a direction. 

Directions are:
[u]p    or [n]orth
[d]own  or [s]outh 
[l]eft  or [w]est 
[r]ight or [e]ast


Example:
u
moves the tile below the empty space up.

Other commands
'quit' - quit the game
'show' - show the current puzzle
'show complete' - show the completed puzzle
]])
        elseif cmd=="quit" then
            sendtochar( ch, "Seeya!\n\r")
            return
        elseif cmd=="show" then
            show_tiles( ch, g.curr )
            show_grid( ch, g.curr )
        elseif cmd=="show complete" then
            show_tiles( ch, g.orig )
        --elseif ch.isimmort and cmd=="win" then
        --    g.orig = g.curr
        elseif string.match(cmd, "^[udlrnsew]") then
            local dir=cmd:sub(1,1)
            local y=g.curr.empty_y
            local x=g.curr.empty_x

            if dir=="u" or dir=="n" then
                y = y+1
            elseif dir=="d" or dir=="s" then
                y = y-1
            elseif dir=="l" or dir=="w" then
                x = x+1
            elseif dir=="r" or dir=="e" then
                x = x-1
            end
            
            if not(g.curr[y]) or not(g.curr[y][x]) then
                sendtochar( ch, "Can't move that direction!\n\r")
            else
                g.curr[g.curr.empty_y][g.curr.empty_x] = g.curr[y][x]
                g.curr[y][x] = nil
                g.curr.empty_y=y
                g.curr.empty_x=x

                -- check if we completed
                local win=true
                for yy=1,TILE_DIMENSION do
                    for xx=1,TILE_DIMENSION do
                        if not(g.curr[yy][xx]==g.orig[yy][xx]) then
                            win=false
                            break
                        end
                    end
                end
                
                if win then
                    local end_time = os.time()
                    check_time(ch, TILE_DIMENSION, start_time, end_time)
                    sendtochar( ch, "YOU WIN!!!!")
                    if TILE_DIMENSION==3 then
                        ch:reward( ch, "qp", 20)
                    elseif TILE_DIMENSION==4 then
                        ch:reward( ch, "qp", 25)
                    elseif TILE_DIMENSION==5 then
                        ch:reward( ch, "qp", 30)
                    end

                    ch:setval( "slide_time", os.time(), true )

                    sendtochar( ch, pic )
                    return
                end

                show_tiles(ch, g.curr)
                show_grid(ch, g.curr)
            end 
        else
            sendtochar( ch, "Invalid command!\n\r")
        end

    end

end

local function try_start_game( ch )
    if ch.isnpc then return end

    local last_time=ch:getval("slide_time") or 0

    --local wait=last_time+79200 - os.time()
    local last_date=os.date("*t",last_time)
    local target_time=os.time({
        year=last_date.year,
        month=last_date.month,
        day=last_date.day+1,
        hour=0,
        minute=0,
        second=0
    })

    if os.time()<target_time then
        sendtochar( ch, "Sorry, you already did the slide puzzle today! Try again tomorrow.\n\r")
        return
    end

    start_con_handler( ch.descriptor, game_con, ch )
end

local function show_leaders(ch)
    sendtochar(ch, "---------SLIDE CHALLENGE LEADERS----------\n\r")
    local lb = area:loadtbl("slide_lb") or {}
    
    for size = 3,5 do
        sendtochar(ch, ("--- %d x %d leaders ---\n\r"):format(size, size))
        
        local leaders = lb[size] or {}
        for i,v in ipairs(leaders) do
            sendtochar( ch,
                string.format("%3d. %-20s %.2f seconds\n\r", i, v.name, v.score))
        end
    end
end

local function show_slide_usage(ch)
    sendtochar(ch, [[
quest slide start -- Start the slide puzzle challenge!
quest slide time  -- Show the top 10 best times for each size!
]])
end

function quest_room_trigger( ch1, ch2, obj1, obj2, text1, trigger, trigtype)
    if text1 == "start" then
        try_start_game(ch1)
        return
    elseif text1 == "time" then
        show_leaders(ch1)
        return
    else
        show_slide_usage(ch1)
    end

    return false
end
