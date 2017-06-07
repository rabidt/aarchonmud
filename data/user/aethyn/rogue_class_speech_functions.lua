-- ROGUE CLASS QUESTS SPEECH FUNCTIONS

-------------------------------------------------------------------------------
-- SILAS                                                                     --
-------------------------------------------------------------------------------


-- FIRST GRALL FOR SILAS. STARTS ROGUE CLASS QUESTS
function silas_first_speech_one()
  say("What's that you got there?")
  sendtochar(lchar,"You give a {ynote of authorization{x to Silas.\n\r")
  lchar:junk("authorization")
  emote("reads the note.")
  say("I heard you might be coming. Bastion won't admit it, but he needs me.")
  lchar:junk("signednote")
  delay(6, silas_first_speech_two)
end


function silas_first_speech_two()
  mdo("ponder")
  say("You and me, we're going to solve this mystery. We'll figure out who started these fires.")
  say("And like all great thieves, we'll be chasing after the money.")
  say("And where can we find the most money? The bank.")
  delay(6, silas_first_speech_three)
end


function silas_first_speech_three()
  say("The Firestarter's Guild will have a box or something.")
  say("You'll break into the bank and find out what they have in it.")
  delay(6, silas_first_speech_four)
end


function silas_first_speech_four()
  say("You can't do this alone. You're going to need a team.")
  say("There's a guy I used to run scams with. He's the best.")
  say("You might have heard of him, his name is El Cid.")
  say("The man has never worked a day in his life, and do you know why?")
  delay(3,function()
    echo("You shake your head.")
    delay(4, function()
      say("It's because he knows how to get people to do his work for him.")
      say("He'll know who you'll need on your team. Go talk to him.")
      say("I'll let him know you're on your way.")
      lchar:setval("r10_story_progress", 6, true)
      for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
        if mobile.vnum == rvnum(413) then
          mobile:destroy()
        end
      end 
    end)
  end)
end


-------------------------------------------------------------------------------
-- EL CID                                                                    --
-------------------------------------------------------------------------------


-- FIRST GRALL FOR EL CID. STARTS HIS PORTION OF THE ROGUE CLASS QUEST
function elcid_first_speech_one()
  say("My old buddy Silas filled me in on our caper.")
  say("To take on Morgan's we'll need a great team.")
  delay(5, elcid_first_speech_two)
end


function elcid_first_speech_two()
  say("We'll need someone to charm our way past the guards who can be charmed,")
  say("and some muscle to get us by those who can't.")
  say("Then we'll need someone who can open up the vault when we get to it.")
  mdo("ponder")
  delay(5, elcid_first_speech_three)
end


function elcid_first_speech_three()
  mdo("snap")
  say("I think I know just who our distraction will be.")
  say("There's an old singer, goes by the name Eddy.")
  say("You can normally find him in the bar at the Dicey Nite's Casino.")
  say("Convince him to join us and we can get going.")
  lchar:setval("r10_story_progress", 7, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end 
end


-------------------------------------------------------------------------------

-- SECOND GRALL FOR CID, AFTER YOU'VE RECRUITED EDDT Caldazar
function elcid_second_speech_one()
  say("Eddy stopped by and told me he's in. Good job.")
  say("We've got our distraction.")
  delay(5, elcid_second_speech_two)
end


function elcid_second_speech_two()
  say("Eddy can deal with the untrained guards, but he won't be able to handle the elites.")
  say("But I know who can. Goes by the name of Knockout Nancy.")
  say("She's currently employed at a hotel, but I'm sure you can convince her to work with us.")
  lchar:setval("r10_story_progress", 10, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
end


-------------------------------------------------------------------------------


-- THIRD GRALL FOR CID, AFTER YOU'VE RECRUITED NANCY
function elcid_third_speech_one()
  say("Nancy told me she's coming along. Great news.")
  say("She's off training now.")
  delay(5, elcid_third_speech_two)
end


function elcid_third_speech_two()
  say("Now, we'll need someone with quick fingers, someone that'll get us into the vault...")
  mdo("ponder")
  mdo("snap")
  say("I know just the guy. He's so good he doesn't even have a name.")
  say("He's known only as the sneaky Thief. Go find him and convince him to join us.")
  lchar:setval("r10_story_progress", 15, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
end


-------------------------------------------------------------------------------

-- FOURTH GRALL FOR CID, AFTER YOU'VE RECRUITED SNEAKY THIEF

-- if you intimidate Renier Grey

function elcid_fourth_speech_intim_one()
  emote("eyes you as you walk in.")
  mdo("sigh")
  say("I heard what happened over at Exotica Imports.")
  say("Renier is going to buy the sneaky thief's diamond, but we have a bigger problem now.")
  delay(4, elcid_fourth_speech_intim_two)
end

function elcid_fourth_speech_intim_two()
  say("Our getaway man has to be Renier.")
  say("He's the only one who can get a tunnel under Morgan's.")
  say("And you just made him very, very angry.")
  delay(4, elcid_fourth_speech_intim_three)
end

function elcid_fourth_speech_intim_three()
  say("Luckily for you he owes me.")
  say("I've already talked to him, and he'll help us.")
  say("But stay away from him. He doesn't like you.")
  delay(4, elcid_fourth_speech_final)
end

-- if you bribe Renier Grey

function elcid_fourth_speech_bribe_one()
  say("Ah, "..lchar.name..". I heard you met Renier Grey.")
  say("That's good, because he's our fourth conspirator.")
  delay(4, elcid_fourth_speech_bribe_two)
end

function elcid_fourth_speech_bribe_two()
  say("He's going to be our getaway man.")
  say("Grey is a master tunneler. He'll get us out from Morgan's when we're ready to go.")
  delay(4, elcid_fourth_speech_final)
end

-- and the final "fourth" speech

function elcid_fourth_speech_final()
  say("The team is assembled then. We're ready to go over our plan.")
  say("Meet me at the private room in the Blind Tiger.")
  say("I'll get everyone together and we'll go over the plan there.")
  emote("walks out onto Smullens Plaza, waving at the drunks.")
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
  -- blind tiger back room
  mob:goto(rvnum(274))
  -- make our co-conspirators vis
  getmobworld(rvnum(455))[1]:setact("wizi", false)
  getmobworld(rvnum(456))[1]:setact("wizi", false)
  getmobworld(rvnum(457))[1]:setact("wizi", false)
  getmobworld(rvnum(458))[1]:setact("wizi", false)
end


-- Now we're going over the plan in the back room of the blind tiger

function elcid_btiger_exposition_one()
  if lchar:getval("r10_intimd_renier") then
    echo("Renier Grey sends you a smouldering gaze.")
  end
  say("Ok, it looks like everyone is here. It's time to go over the plan.")
  say("We need to get in Morgan's vaults. We're looking for documents.")
  emote("looks pointedly at a sneaky Thief.")
  mdo("smile")
  say("But of course whatever you can get your hands on is yours.")
  delay(8, elcid_btiger_exposition_two)
end

function elcid_btiger_exposition_two()
  echo("{sA sneaky Thief says {S'Knocking off old Morgan, wow.'{x")
  echo("{sA sneaky Thief says {S'I always thought about it. Never thought I'd get the chance.'{x")
  delay(5, elcid_btiger_exposition_three)
end

function elcid_btiger_exposition_three()
  echo("{sRenier Grey says {S'Please, you wouldn't know what to do if you ever got in there.'{x")
  echo("{sKnockout Nancy says {S'Oh come on Renier, lighten up.'{x")
  echo("Knockout Nancy winks at a sneaky Thief.")
  echo("A sneaky Thief blushes.")
  delay(8, elcid_btiger_exposition_four)
end

function elcid_btiger_exposition_four()
  say("If you three are quite done, I think "..lchar.name.." is ready to hear the plan.")
  lchar:mdo("nod")
  say("And it would help if you bunch understood your roles too.")
  echo("Eddy Caldazar nods.")
  delay(8, elcid_btiger_exposition_five)
end

function elcid_btiger_exposition_five()
  say("Now, we all know what the entrance to the bank looks like...")
  lchar:at(rvnum(86).." look")
  delay(8, elcid_btiger_exposition_six)
end

function elcid_btiger_exposition_six()
  say("But it's changed. With all the unrest Morgan has added extra guards...")
  for _,guard in ipairs(getmobworld(rvnum(461))) do -- let's make sure our guards are visible now
    guard:setact("wizi", false)
  end
  lchar:at(rvnum(86).." look") -- and there they are!
  delay(8, elcid_btiger_exposition_seven)
end

function elcid_btiger_exposition_seven()
  say("They're under strict orders to not let anyone into the small box rooms.")
  say("Eddy, this is where you come in. Those guards won't have had any training.")
  say("Most will have been pulled out of the bars in Smullens Plaza,")
  say("and we all know that's your crowd.")
  delay(8, elcid_btiger_exposition_eight)
end

function elcid_btiger_exposition_eight()
  echo("{sEddy Caladazar says {S'Buddy, my crowd is anyone who like good music.'{x")
  echo("Renier Grey snorts.")
  say("Moving on...")
  delay(8, elcid_btiger_exposition_nine)
end

function elcid_btiger_exposition_nine()
  say("Now I won't be going in with you, but...")
  echo("{sKnockout Nancy mutters {S'Big surprise.'{x")
  echo("{sEl Cid continues, ignoring Nancy, {S'...but I will be sending you instructions telepathically.'{x")
  say("Unfortunately Morgan has hired a mage to stop all telepathic communication.")
  say("The mage has placed a rune on the floor of the hallway leading to the small box rooms.")
  getroom(rvnum(364)):loadfunction(function() rune_on = true end)
  lchar:at(rvnum(364).." look")
  delay(10, elcid_btiger_exposition_ten)
end

function elcid_btiger_exposition_ten()
  say("Luckily for us it's very easily destroyed.")
  say("Once Eddy gets you past the first set of guards "..lchar.name.." will have to destroy the rune.")
  say("This has the added bonus of blowing a tunnel into the lower vaults, which is where we want to be.")
  say("Mr. Grey will supply you with the bomb.")
  delay(10, elcid_btiger_exposition_eleven)
end

function elcid_btiger_exposition_eleven()
  say("Now this bomb is a shaped charge, so you don't have to worry about leaving the room when it blows,")
  say("but I doubt Morgan's regular guards are going to just let you plant it.")
  say("Nancy will take care of them as only she knows how.")
  echo("Knockout Nancy blows El Cid a kiss.")
  delay(10, elcid_btiger_exposition_twelve)
end

function elcid_btiger_exposition_twelve()
  say("Once it blows the rune should be gone and you'll have a way into the lower levels.")
  say("At that point I'll resume my communication.")
  say("I know there are vaults down there, and I know our sneaky Thief here can get us into them.")
  say("What I don't know is which box room our information will be in and which box we'll need.")
  say("There are three rooms, with three boxes each...")
  delay(10, elcid_btiger_exposition_thirteen)
end

function elcid_btiger_exposition_thirteen()
  lchar:at(rvnum(372).." look")
  delay(6, elcid_btiger_exposition_fourteen)
end

function elcid_btiger_exposition_fourteen()
  say("...and some boxes will be more heavily guarded than others.")
  say("While I don't know which box we want, I can say this:")
  say("I know all the people who warrant high security boxes, and I can say they're not behind this fire.")
  say("This is important. High security boxes will bring extra guards or be trapped.")
  delay(10, elcid_btiger_exposition_fifteen)
end

function elcid_btiger_exposition_fifteen()
  say("When "..lchar.name.." has found what we're looking for he'll get back to me.")
  say("Then Mr. Grey will blow a hole in the vault floor, leading us back to Silas")
  say("where we'll go over the information and whatever other prizes we found.")
  delay(10, elcid_btiger_exposition_sixteen)
end

function elcid_btiger_exposition_sixteen()
  say("Everyone understand their role?")
  lchar:mdo("nod")
  echo("Knockout Nancy cracks her knuckles.")
  echo("Eddy Caldazar starts humming.")
  echo("{sRenier Grey says {S'Looks like that sneaky Thief already left. Guess he's ready.'{x")
  echo("{sRenier Grey says {S'I know I am.'{x")
  sthief:setact("wizi")
  delay(10, elcid_btiger_exposition_seventeen)
end

function elcid_btiger_exposition_seventeen()
  say("Good. Let's go.")
  mdo("open east")
  echo("Knockout Nancy leaves east.")
  echo("Eddy Caldazar leaves east.")
  echo("Renier Grey leaves east.")
  eddy:setact("wizi")
  nancy:setact("wizi")
  renier:mdo("east")
  lchar:setval("r10_story_progress", 18, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
end

-------------------------------------------------------------------------------
-- EDDY CALDAZAR                                                             --
-------------------------------------------------------------------------------


-- FIRST GRALL FOR EDDY. START HIS PORTION OF THE ROGUE CLASS QUEST
function eddy_first_speech_one()
  say("I told you, I'll have your money, but I can't make any when there's no customers!")
  echo("{sAn agitated mafioso says {S'We're tired of waiting Eddy.'{x")
  say("I didn't start a fire and get this place closed down, what do you want me to do?")
  emote("looks at you as you enter.")
  say("Look, an audience, let me sing a few songs, I'll get you your money.")
  delay(5, eddy_first_speech_two)
end


function eddy_first_speech_two()
  echo("An agitated mafioso looks you over.")
  echo("{sAn agitated mafioso says {S'"..lchar.heshe:gsub("^%l", string.upper).." don't look like "..lchar.heshe.." has enough to pay off what you owe.'{x")
  say("No, no, I got this.")
  emote("starts singing.")
  delay(5, eddy_first_speech_three)
end


function eddy_first_speech_three()
  echo("You clear your throat.")
  echo("{sYou say {S'Eddy Caldazar, Cid sent me. We want you for a job.{x")
  emote("looks relieved.")
  delay(5, eddy_first_speech_four)
end


function eddy_first_speech_four()
  say("I'd love to help, I really would. But my esteemed colleague here, he might have an issue with me leaving right now.")
  echo("An agitated mafioso nods.")
  echo("{sAn agitated mafioso says {S'Look pal, unless you're here to pay me, get lost.'{x")
  delay(5, eddy_first_speech_five)
end


function eddy_first_speech_five()
  say("So you see, my services can be purchased for the 5,000 gold I owe this gentleman up front, plus 20% of the cut, whatever it is.")
  echo("{tEddy Caldazar tells you {T'Or you could just, erm, get rid of him for me.'{x")
  echo("{tEddy Caldazar tells you {T'Kill him and I'll work for you for 18% of the cut.'{x")
  echo("An agitated mafioso glares at you.")
  echo("You can pay off Eddy's debts by giving 5,000 gold to the mafioso or you can kill him.")
  lchar:setval("r10_story_progress", 8, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
end

-------------------------------------------------------------------------------


-- SECOND SPEECHES FOR EDDY, COMES AFTER MAFIOSO IS KILLED OR BRIBED
function eddy_second_speech_one()
  say("Wow, you got rid of him for me. I appreciate it.")
  say("So what's this job?")
  delay(3,function() lchar:say("We're going to rob Morgan's.") end)
  delay(7, eddy_second_speech_two)
end


function eddy_second_speech_two()
  mdo("whistle")
  say("That's quite the job. Ok, I'm in. Tell Cid to call me when we're ready to go.")
  say("He knows how to reach me.")
  emote("walks out of the bar.")
  lchar:setval("r10_story_progress", 9, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
  mob:setact("wizi")
  mob:goto(rvnum(399))
end


-------------------------------------------------------------------------------


-- EDDY SPEECH IN THE BANK

function eddy_bank_speech_one()
  echo("One of the guards looks at you and shakes his head.")
  echo("{sA guard says {S'Sorry buddy, no one is allowed back there by order of the boss.'{x")
  delay(3, eddy_bank_speech_two)
end

function eddy_bank_speech_two()
  echo("{tEl Cid tells you {T'And here comes Eddy in three...'{x")
  delay(1, function() 
    echo("{tEl Cid tells you {T'Two'{x")
    delay(1, function()
      echo("{tEl Cid tells you {T'One'{x")
      delay(1, function()
          getmobworld(rvnum(463))[1]:setact("wizi", false)
          echo("Eddy Caldazar has arrived.")
          delay(3, eddy_bank_speech_three)
      end)
    end)
  end)
end

function eddy_bank_speech_three()
  echo("{sOne of the guards screams, {S'Rimbol's ripe ass! Is that Eddy Caldazar?{x")
  echo("{sEddy Caldazar asks {S'Fellas, fellas, it is me. How about a song?'{x")
  emote("nods their heads in unison.")
  delay(5, eddy_bank_speech_four)
end

function eddy_bank_speech_four()
  echo("Eddy Caldazar winks at you.")
  echo("Eddy starts singing as he walks out the bank...")
  echo("...and the guards follow him!")
  echo("{sLenny the Moneychanger exclaims {S'Wait! Where are you going? Come back!'{x")
  getmobworld(rvnum(463))[1]:setact("wizi")
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
  mob:goto(rvnum(399))
  delay(2, function()
    sendtochar(lchar, "{tEl Cid tells you {T'Now get rid of that rune and we'll be in touch.'{x\n\r")
  end)
end


-------------------------------------------------------------------------------
-- KNOCKOUT NANCY                                                            --
-------------------------------------------------------------------------------


-- FIRST SPEECHES FROM NANCY WHEN YOU FIRST SEE HER
function nancy_first_speech_one()
  say("Eddy stopped by. He told me what's up.")
  say("I'd love to help, but I work for Madame Carmilla.")
  say("I owe her 5,000 gold. You're going to have to pay her if I'm going to be able to leave here.")
  say("Go talk to her. I'm sure someone as handsome and strong as you can work something out.")
  lchar:setval("r10_story_progress", 11, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
  delay(10, function()
    mob:setact("wizi")
    mob:goto(rvnum(399))
  end)
end


-------------------------------------------------------------------------------


-- SECOND SPEECH FOR NANCY FOR AFTER YOU DEAL WITH MADAME CARMILLA
function nancy_second_speech_one()
  mdo("wake "..lchar.name)
  say("I'm waiting to hear back from you and here you are, sleeping!")
  mdo("roll")
  say("Go tell Cid I'll be waiting for his call.")
  emote("walks off in disgust.")
  lchar:setval("r10_story_progress", 14, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
  mob:setact("wizi")
  mob:goto(rvnum(399))
end


-------------------------------------------------------------------------------
-- SNEAKY THIEF                                                              --
-------------------------------------------------------------------------------


-- FIRST SPEECH FOR SNEAKY THIEF WHEN YOU FIRST GET TO SNEAKY THIEF
function sneakythief_first_speech_one()
  lchar:say("We want you.")
  delay(3, sneakythief_first_speech_two)
end


function sneakythief_first_speech_two()
  say("I heard you and Cid are trying to raise a gang.")
  say("I'd love to help, but I have problems of my own.")
  say("I've got what we might call a, erm, hot diamond.")
  delay(5, sneakythief_first_speech_three)
end


function sneakythief_first_speech_three()
  say("My regular fence, Renier Grey, won't take it.")
  say("Get him to take it and I'll help.")
  say("Once you're done with that, just go back to Cid. He knows where to find me.")
  lchar:setval("r10_story_progress", 16, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
  mob:goto(rvnum(399))
end


-------------------------------------------------------------------------------
-- RENIER GREY                                                               --
-------------------------------------------------------------------------------


-- FIRST GRALL FOR RENIER GREY AFTER SEEING SNEAKY THIEF
function renier_sneaky_first_speech_one()
  say("I know why you're here.")
  say("That sneaky thief wants me to take that diamond of his.")
  delay(3, renier_sneaky_first_speech_two)
end


function renier_sneaky_first_speech_two()
  say("That thing is way too hot and he wants way too much.")
  say("If you want me to take it, you'll have to sweeten the deal.")
  mdo("smirk")
  say("I think 5,000 gold will be enough, considering the risk I'm taking.")
  delay(3, renier_sneaky_first_speech_three)
end


function renier_sneaky_first_speech_three()
  mdo("glare "..lchar.name)
  say("Give me the gold or leave. This isn't a negotiation.")
  if lchar:getval("r10_rogue_miniboss_kills") > 1 then
    echo("You can either intimidate Renier or give him the gold.")
  else
    echo("You have no more patience or gold to give. Your only option is to intimidate Renier.")
  end
end


-- INTIMIDATATION SPEECH

function renier_speech_intim_one()
  echo("You pull out a dagger and begin flipping it.")
  echo("Without a word you catch it and toss it...")
  delay(2, renier_speech_intim_two)
end

function renier_speech_intim_two()
  echo("{Y...THUNK!{x")
  echo("The dagger embeds itself in the wall an inch from Renier Grey's left ear.")
  delay(4, renier_speech_intim_three)
end

function renier_speech_intim_three()
  say("Ok, ok! Fine, I'll buy his stupid diamond.")
  mdo("fume")
  say("But I have powerful friends. You better watch your back.")
  say("Now get out of my shop.")
  emote("removes your dagger from the wall and begins to clean his nails with it.")
  lchar:setval("r10_rogue_miniboss_kills", lchar:getval("r10_rogue_miniboss_kills")+1, true)
  lchar:setval("r10_story_progress", 17, true)
  lchar:setval("r10_intimd_renier", true, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
end


-- BRIBERY SPEECH

function renier_speech_bribe_one()
  emote("smiles as you hand over the coin.")
  say("Ok, I'll take that shifty thief's diamond. Have him send it over.")
  say("I'm glad we could come to an agreement.")
  emote("turns his back and inspects his wares.")
  lchar:setval("r10_story_progress", 17, true)
  lchar:setval("r10_intimd_renier", false, true)
  for _,mobile in ipairs(mob.room.mobs) do -- destroy the mob that prevents you from leaving
    if mobile.vnum == rvnum(413) then
      mobile:destroy()
    end
  end
end