------------------------------------------------------------------------------------------------
-- I'LL TAKE YOU RIGHT INTO THE DANGER ZONE                                                   --
------------------------------------------------------------------------------------------------

local nav_table = {}
for i=1,12 do
  nav_table[i] = { "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|",
          "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|",
          "_", "_", "_" }
end
nav_table['home'] = { " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|",
          " ", "{CY{x", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|",
          " ", " ", " " }
nav_table[1][20] = "{R|"
nav_table[1][24] = "|{x"

nav_table[2][20] = "{R|"
nav_table[2][24] = "|{x"

nav_table[3][20] = "{Y|"
nav_table[3][24] = "|{x"

nav_table[4][20] = "{Y|"
nav_table[4][24] = "|{x"

nav_table[5][20] = "{Y|"
nav_table[5][24] = "|{x"

function start_school()
  mob:say("Welcome to flight school. I'm your instructor, Kaia.")
  mob:say("The planes we fly in the Galaxy Fleet are your standard biplanes. They seat one pilot and one pilot only.")
  mob:say("If you need to take someone other than yourself with you they must fly on the wings. But the biplane will only be able to hold 3 people total.")
  mob:say("Now we'll go over the controls in your plane.")
  delay(7, second_part)
end

function second_part()
  mob:say("Before you enter the cockpit the mechanics aboard the fleet will have your biplane running and ready to go.")
  mob:say("When you enter the cockpit you'll see a lever. This lever will detach the plane from the Excelsior, the airship where your plane will be docked.")
  mob:say("Pull the lever to start the plane and we'll continue the lesson.")
  mob:mload(9339)
  mob:echo("A bright, {RRED{x lever appears.")
  -- the player pulls the lever to start third_part()
end

function third_part()
  mob:say("Now that you're in your plane you'll see a grid. This is your flight map. It looks like this:")
  mob:echo(" ")
  for i=12,1,-1 do
    mob:echo(table.concat(nav_table[i], ""))
  end
  mob:echo(table.concat(nav_table['home'], "")) -- Always displays home line, showing your flyer
  mob:echo(" ")
  delay(7, fourth_part)
end

function fourth_part()
  mob:say("See the \"{CY{S\"? That's you! You'll always be on the bottom, though you will move side to side.")
  mob:say("What about the colored spots on the map? That's your aiming reticle.")
  mob:say("Inside the yellow lines an enemy may shoot you. Inside the red, you can shoot an enemy.")
  mob:say("What do enemies look like? On your map they're shown as \"{M#{S\"")
  mob:say("Here's a map with one enemy plane:")
  nav_table[7][18] = "{M#{x"
  mob:echo(" ")
  for i=12,1,-1 do
    mob:echo(table.concat(nav_table[i], ""))
  end
  mob:echo(table.concat(nav_table['home'], "")) -- Always displays home line, showing your flyer
  mob:echo(" ")
  delay(9, fifth_part)
end

function fifth_part()
  mob:say("To move left or right you can go west or east respectively. The plane will interpret this as a movement of the stick.")
  mob:say("For instance, in the last example moving west one space gives this new map:")
  nav_table[7][18] = "{M#{x"
  
  nav_table[1][20] = "|"

  nav_table[2][20] = "|"

  nav_table[3][20] = "|"

  nav_table[4][20] = "|"

  nav_table[5][20] = "|"

  nav_table[1][16] = "{R|"
  nav_table[1][20] = "|{x"

  nav_table[2][16] = "{R|"
  nav_table[2][20] = "|{x"

  nav_table[3][16] = "{Y|"
  nav_table[3][20] = "|{x"

  nav_table[4][16] = "{Y|"
  nav_table[4][20] = "|{x"

  nav_table[5][16] = "{Y|"
  nav_table[5][20] = "|{x"

  nav_table['home'][22] = " "
  nav_table['home'][18] = "{CY{x"

  mob:echo(" ")
  for i=12,1,-1 do
    mob:echo(table.concat(nav_table[i], ""))
  end
  mob:echo(table.concat(nav_table['home'], "")) -- Always displays home line, showing your flyer
  mob:echo(" ")

  mob:say("Going east would take you in the opposite direction.")
  delay(12, sixth_part)
end

function sixth_part()
  mob:say("If you want to shoot at a plane, you go north. North will tell the plane you've pulled the trigger.")
  mob:say("You can shoot at any time, and your ammunition is unlimited, but you can only hit planes in your red reticle.")
  mob:say("But don't line them up too early. The enemy has greater range than you, and can shoot you from inside the yellow reticle.")
  mob:say("A plane you can hit looks like this:")
  nav_table[7][18] = "_"
  nav_table[2][18] = "{M#{R"
  for i=12,1,-1 do
    mob:echo(table.concat(nav_table[i], ""))
  end
  mob:echo(table.concat(nav_table['home'], "")) -- Always displays home line, showing your flyer
  mob:echo(" ")
  delay(10, seventh_part)
end

function seventh_part()
  mob:say("In addition to the map you will see a status gauge. This gauge gives you updates on how much health your plane has left, how many enemy planes you shot down and how many enemy planes are left.")
  mob:say("It'll look like this:")
  mob:echo(" ")
  mob:echo("{WYour plane's health is: {R1000{x")
  mob:echo("{WYou've destroyed {y0{W/{Y100{W planes.{x")
  mob:echo("{WThere are {M200{W planes left.{x")
  mob:echo(" ")
  mob:say("You start with 1000 health. There are three ways to lose it.")
  mob:say("The first is to get shot by a plane. The enemy has weak armaments and their attacks only do 10 damage.")
  mob:say("Their attacks will only hit you when they're within the yellow and red reticles of your flight map.")
  mob:say("The second way to lose health is to run into planes. If you don't shoot an enemy plane down and are on the same path as it you will lose 250 health.")
  mob:say("We'll get to the third way in a bit.")
  mob:say("When you reach zero health your plane will crash and your parachute will carry you to safety, but your mission will have failed.")
  delay(13, eighth_part)
end

function eighth_part()
  mob:say("In order to ensure there won't be enough planes to circle back on you you'll need to shoot down 100 planes.")
  mob:say("Your goal is to get to the Warlord's blimp. His blimp looks like \"{G@{S\"")
  mob:say("On the map it looks like this:")
  nav_table[2][18] = "_"
  nav_table[11][18] = "{G@{x"
  for i=12,1,-1 do
    mob:echo(table.concat(nav_table[i], ""))
  end
  mob:echo(table.concat(nav_table['home'], "")) -- Always displays home line, showing your flyer
  mob:echo(" ")
  mob:say("Running your plane into the blimp will take you safely into it, assuming you've shot down enough enemy planes. This is your goal.")
  delay(12, ninth_part)
end

function ninth_part()
  mob:say("Now, that's all you need to know about flying. But, as you should be bringing a commando team with you into the Warlord's ship, there's something else you must know.")
  mob:say("The Warlord has under his control a group of gargoyles who he uses to fly onto wings. From the cockpit you will not be able to attack them.")
  mob:say("But, anyone who is on the wings will be able to fight them in a traditional manner. After several seconds they will sacrifice themselves to do damage to your wings.")
  mob:say("This will remove 100 health from your plane, so your comrades must destroy them.")
  mob:say("That is all you need to know for your mission to the Warlord's ship. I am proud to award you your pilot's license. May fortune be with you.")
  mob:say("Head over to the Excelsior and launch for the Warlord.")
  randchar():setval("galaxy pilot's license", 1, true)
  mob:oload(9325)
  mdo("give license %s", randchar().name)
  delay(5, function()
    mob:transfer("all 9323")
    getmobworld(9340)[1]:loadfunction(function () mob:destroy() end) -- destroys the mob that I can't seem to purge?
    mob:goto(9323)
    mob:echo(mob.shortdescr.." {Wwalks in behind you.{x")
  end)
end