------------------------------------------------------------------------------------------------
-- Biplane class functions. For flying.																	                      --
------------------------------------------------------------------------------------------------
Biplane = {}
Biplane.__index = Biplane


local transfer_vnums = { 9372, 9373, 9374, 9375 }

-- local checktime = nil
-- local checkdelay = 3

function Biplane.create()
	lat_counter = 6 -- this will keep the plane within +/-5 of its starting point laterally, goes from 1 to 11
	flyer_pos = 22
	scroll_counter = 1
	scroll_number = 400 -- number of total lines
	health = 1000
	plane_kills = 0
	num_planes_killed = 100

	-- Creates the table for the map. Sets metatable for object-like action
	local nav_table = {}
	setmetatable(nav_table,Biplane)
	for i=1,scroll_number+30 do
		nav_table[i] = { "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|",
	 				 	"_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|", "_", "_", "_", "|",
					 	"_", "_", "_" }
	end
	nav_table['home'] = { " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|",
					  " ", "{CY{x", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|", " ", " ", " ", "|",
					  " ", " ", " " }
	return nav_table
end

function Biplane.print_map(self)
	-- Prints map, uses loop to print 13 lines at a time
	mob:echo(" ")
	for i=12,0,-1 do
		--for debug (shows line numbers):
		--mob:echo(i+scroll_counter)
		--end debug
		mob:echo(table.concat(self[i+scroll_counter], ""))
	end
	mob:echo(table.concat(self['home'], "")) -- Always displays home line, showing your flyer
	mob:echo(" ")
	mob:echo("{WYour plane's health is: {R"..health.."{x")
	mob:echo("{WYou've destroyed {y"..plane_kills.."{W/{Y"..num_planes_killed.."{W planes.{x")
	mob:echo("{WThere are {M"..plane_counter.."{W planes left.{x")
end

function Biplane.populate_map(self, num_of_planes)
	-- Populates map. Any number of planes, with random positioning or set
	num_of_planes = num_of_planes or 1
	plane_counter = num_of_planes
	for i=1,num_of_planes do
		x = 2 + 4*randnum(0, 10) -- x coordinate for planes
		y = randnum(10, scroll_number - 5) -- y coordinate for planes, no planes in first 10 or last 5 lines
		-- for debug (shows plane positions):
		-- mob:echo(i..":  "..x..",  "..y)
		-- end debug
		-- create biplanes
		if self[y][x] == "{M#{x" then
			-- mob:echo("Moved a plane")
			self[y+randnum(-1,1)][2+4*randnum(0,10)] = "{M#{x"
		else
			self[y][x] = "{M#{x"
		end
	end
	self[scroll_number][2 + 4*randnum(0,10)] = "{G@{x" -- creates blimp somewhere on last line
	self:trans_down(1)
end

function Biplane.trans_down(self, y)
	-- Makes the map scroll down, making flyer appear to go up
	scroll_counter = scroll_counter + y
	for i,j in ipairs(self) do
		for k,v in ipairs(j) do
			if v == "{M#{x" then
				if i < scroll_counter then -- check for plane collision
					if k == flyer_pos then
						mob:echo("{RYou've hit a plane!{x")
						self[i][k] = "_"
						health = health - 250
					else
						mob:echo("{WA plane got away!{x")
						self[i][k] = "_"
					end
					plane_counter = plane_counter - 1
				--else -- Might not want this
				--	move_chance = randnum(1,100)
				--	if k < 22 and i < scroll_counter + 6 and move_chance < 40 then -- planes seek middle of map, to shoot at target
				--		self[i][k] = "_"
				--		self[i][k+4] = "I" -- intermediate symbol so it can complete loop
				--	elseif k > 22 and i < scroll_counter + 6 and move_chance < 40 then
				--		self[i][k] = "_"
				--		self[i][k-4] = "I"
				--	end
				end
			end
			if v == "{G@{x" then
				if k == flyer_pos and scroll_counter > scroll_number then
					if plane_kills >= num_planes_killed then
						for _,b in ipairs(transfer_vnums) do -- transfer people on wings
							mob:at("%d mob echo {WYou've successfully docked with the {DWARLORD's{W ship.{x", b)
							mob:at("%d mob transfer all 9376", b)
						end
						mob:goto(9377)
						mob:purge()
						mob:mload(9318)
						mob:destroy()
						return
					else
						for _,b in ipairs(transfer_vnums) do -- transfer people on wings
							mob:at("%d mob echo {WAs you reach the airship, the planes that escaped your fire circle around to shoot you down.{x", b)
							mob:at("%d mob transfer all 10204", b)
						end
						mob:destroy()
						return
					end
				elseif scroll_counter > scroll_number then
					for _,b in ipairs(transfer_vnums) do -- transfer people on wings
							mob:at("%d mob echo {WYou've missed the ship! As you run out of fuel you bail out and parachute to safety.{x", b)
							mob:at("%d mob transfer all 10204", b)
					end
					mob:destroy()
					return
				end
			end
			if v == "{R|" or v == "{Y|" then
				self[i][k] = "{x|"
			end
			if v == "{R_" or v == "{Y_" then
				self[i][k] = "{x_"
			end
		end
		for l,m in ipairs(j) do
			if m == "I" then
				self[i][l] = "{M#{x" -- returns back I to {M#{x for biplanes
			end
		end
	end
	for count=0,1 do
		self[scroll_counter+count][flyer_pos - 2] = "{R|"
		self[scroll_counter+count][flyer_pos - 1] = "{R_"
		self[scroll_counter+count][flyer_pos + 1] = "{R_"
		self[scroll_counter+count][flyer_pos + 2] = "|{x"
	end
	for count=2,5 do
		self[scroll_counter+count][flyer_pos - 2] = "{Y|"
		self[scroll_counter+count][flyer_pos - 1] = "{Y_"
		self[scroll_counter+count][flyer_pos + 1] = "{Y_"
		self[scroll_counter+count][flyer_pos + 2] = "|{x"
	end
	self:enemy_fire()
	self:print_map()
end

function Biplane.trans_lat(self, x)
	-- trans plane laterally
	lat_counter = lat_counter + x
	if lat_counter >= 1 and lat_counter <= 11 then
		self['home'][flyer_pos] = " "
		self['home'][flyer_pos+4*x] = "{CY{x"
		flyer_pos = flyer_pos + 4*x
	else
		mob:echo("{YYou can't move over that far!{x")
		lat_counter = lat_counter - x
	end
	for i,j in ipairs(self) do -- Change former reticle back to norm
		for k,v in ipairs(j) do
			if v == "{R|" or v == "{Y|" then
				self[i][k] = "{x|"
			end
			if v == "{R_" or v == "{Y_" then
				self[i][k] = "{x_"
			end
		end
	end
	for count=0,1 do
		self[scroll_counter+count][flyer_pos - 2] = "{R|"
		self[scroll_counter+count][flyer_pos - 1] = "{R_"
		self[scroll_counter+count][flyer_pos + 1] = "{R_"
		self[scroll_counter+count][flyer_pos + 2] = "|{x"
	end
	for count=2,5 do
		self[scroll_counter+count][flyer_pos - 2] = "{Y|"
		self[scroll_counter+count][flyer_pos - 1] = "{Y_"
		self[scroll_counter+count][flyer_pos + 1] = "{Y_"
		self[scroll_counter+count][flyer_pos + 2] = "|{x"
	end
	self:print_map()
end

function Biplane.shoot(self)
	-- Player to shoot at incoming planes
	if self[scroll_counter][flyer_pos] == "{M#{x" or self[scroll_counter+1][flyer_pos] == "{M#{x" then -- If a plane is within 2 spaces in front of player
		self[scroll_counter][flyer_pos], self[scroll_counter+1][flyer_pos] = "_", "_"
		--better without print map I think, let trans_down do it
		--self:print_map()
		mob:echo("{MYou hit the plane!{x")
		plane_counter = plane_counter - 1
		plane_kills = plane_kills + 1
	else -- no plane in target area
		--better without print map I think, let trans_down do it
		--self:print_map()
		mob:echo("{MThere's nothing to hit!{x")
	end
end

function Biplane.enemy_fire(self)
	-- checks to see if an enemy plane is in range, then fires, 50% chance to hit
	for i=0,5 do
		if self[scroll_counter+i][flyer_pos] == "{M#{x" then
			mob:echo("{RAn enemy plane is shooting at you!{x")
			rnum = randnum(1,100)
			if rnum < 50 then
				health = health - 10
			end 
		end
	end
end

-- DEPRECATED
--function Biplane.check_time(self, t)
--	
--	local checktime=nil
--	local checkdelay = t or 3 -- check's time
--	
--	local currtime=os.time()
--  	if checktime==nil then
--   	checktime=currtime+checkdelay
--    	return
--  	elseif currtime<checktime then
--    	return
--  	end
--
--	say("Hello Hello")
--	checktime=currtime+checkdelay
--end

------------------------------------------------------------------------------------------------
-- Non-class helper functions for flying 																											--
------------------------------------------------------------------------------------------------

function timed_move()
	if scroll_counter < scroll_number + 1 then
		if health > 0 then
    	bp:trans_down(1)
    	delay(0.75, timed_move, "move")
    else
    	for _,b in ipairs(transfer_vnums) do -- transfer people on wings
    		mob:at("%d mob echo {WYour plane goes down in {RFLAMES{W!{x", b)
				mob:at("%d mob transfer all 10204", b)
			end
 			cancel("move")
 			cancel("wing_timer")
 			mob:destroy()
    end
  else
    say("it's over bro, this has to change to end it in favor or not. Trans to PS or the other blimp")
    cancel("move")
    cancel("wing_timer")
    return
  end
end

------------------------------------------------------------------------------------------------
-- Functions for biplane wings																																--
------------------------------------------------------------------------------------------------

-- vnums are in this order: center, starboard, port
wing_vnums = { 9373, 9374, 9375 }
wings_name = { "Center", "{GStarboard{x", "{RPort{x" }

function wing_timer()
	if health > 0 then
		r = randnum(1,3)
		mob:at("%d mob mload 9313", wing_vnums[r])
		for _,v in ipairs(wing_vnums) do
			mob:at("%d mob echo A gargoyle appears on the " .. wings_name[r] .. " Wing!", v)
		end
		--say("timer up")
		delay(18, wing_timer, "wing_timer")
	else
		cancel("wing_timer")
	end
end