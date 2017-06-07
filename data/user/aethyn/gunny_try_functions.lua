-- FUNCTIONS FOR TRY MPROG FOR INTERROGATION GUNSLINGER QUEST
-- let's get some of these functions eh?
try_functions = true -- so we can be sure we loaded them

function mean_speech_one()
  say("You know, if it were up to me I'd let you go. I know underlings like you don't have a lot of information.")
  say("But "..lchar.name.." over there, he thinks you know something.")
  say("He says you know who put you up to setting all those fires.")
  echo("You crack your knuckles.")
  delay(3, function()
    echo("{sA tied-up firestarter lieutenant exclaims {S'I swear! I don't know who hired us!'{x")
    echo("{sA tied-up firestarter lieutenant exclaims {S'I was just following orders. Please don't hurt me!{x")
  end)
  delay(7, mean_speech_two)
end

function mean_speech_two()
  lchar:say("You know what? I don't trust you and I don't like you.")
  lchar:say("I hate liars. Just hate them.")
  echo("You approach a tied-up firestarter lieutenant and punch him in the mouth.")
  delay(5, mean_speech_three)
end

function mean_speech_three()
  emote("A tied-up firestarter lieutenant sobs.")
  echo("{sA tied-up firestarter lieutenant says {S'All I saw was a guy dropping off money every week at the Chief's office.'{x")
  echo("{sA tied-up firestarter lieutenant exclaims {S'I honestly don't know anything more! You have to believe me!'{x")
  echo("You can \"{Ctry believe{x\" or \"{Ctry disbelieve{x\" the firestarter lieutenant.")
end

function believe_speech_one()
  echo("You nod at {yAl{x the wicked.")
  say("I believe you, I really do, but my friend here doesn't.")
  say("I don't want him to get at you again, but if you can even tell me who was dropping off that money,")
  say("I'll have to let my friend hurt you some more.")
  emote("comforts a tied-up firestarter lieutenant.")
  delay(2, function()
    echo("A tied-up firestarter lieutenant glares at you.")
  end)
  delay(7, believe_speech_two)
end

function believe_speech_two()
  echo("{sA tied-up firestarter lieutenant says {S'I know his name. It's that crazy guy, the one that's always around the candy store.'{x")
  echo("{sA tied-up firestarter lieutenant says {S'Uncle Carl, that's his name'{x")
  delay(5, believe_speech_three)
end

function believe_speech_three()
  say("I think that's all he knows hoss.")
  lchar:mdo("nod")
  say("What do you want to do with him?")
  echo("You can either \"{Ckill firestarter{x\" now or \"{Csay let him go{x\".")
  getmobworld(rvnum(418))[1]:setact("safe", false)
  lchar:setval("r10_story_progress", 11, true)
end

function disbelieve_speech_one()
  echo("You pull out your gun and shoot a tied-up firestarter lieutenant in the kneecap.")
  echo("A tied-up firestarter lieutenant screams!")
  delay(5, disbelieve_speech_two)
end

function disbelieve_speech_two()
  lchar:say("I told you I hate liars.")
  lchar:say("If you want to have two kneecaps when we're finished here you better start talking.")
  delay(5,disbelieve_speech_three)
end

function disbelieve_speech_three()
  echo("A tied-up firestarter lieutenant sobs.")
  echo("{sA tied-up firestarter lieutenant says {S'All I know is the guy's name! That crazy guy at the candy shop.'{x")
  echo("{sA tied-up firestarter lieutenant says {S'Uncle Carl, he's the one who dropped off the cash! That's all I know.'{x")
  delay(4, function()
    say("That has to be all he knows hoss.")
    lchar:mdo("nod")
    say("What do you want to do with him?")
    echo("You can either \"{Ckill firestarter{x\" now or \"{Csay let him go{x\".")
    lchar:setval("r10_story_progress", 11, true)
  end)
end

function nice_speech_one()
  lchar:say("Look, I'm a reasonable person, but Al here, he's not.")
  emote("grins and continues cleaning his gun.")
  delay(1, function() echo("A tied-up firestarter lieutenant looks at {yAl{x the wicked suspiciously.") end)
  delay(5, nice_speech_two)
end

function nice_speech_two()
  lchar:say("I'm on your side. I don't want Al to have his chance at you. Just tell me who hired you to set those fires.")
  delay(4, function()
    echo("{sA tied-up firestarter lieutenant exclaims {S'I'm just an underling! I don't have any information!'{x")
    echo("{sA tied-up firestarter lieutenant says {S'Please don't give me to Al, I swear I can't tell you anything'{x")
    echo("You can \"{Ctry sympathize{x\" or \"{Ctry indifference{x\" with the firestarter lieutenant.")
  end)
end

function sympathize_speech_one()
  lchar:say("I really want to help you out here, but Al, he's demanding we get something out of you.")
  mdo("grin")
  delay(3,function()
    lchar:say("Just give me something, anything, or I'll have to let Al have his way.")
    echo("A tied-up firestarter lieutenant sobs.")
  end)
  delay(7, sympathize_speech_two)
end

function sympathize_speech_two()
  say("He doesn't know anything. Let me just get rid of him.")
  emote("pulls out a knife.")
  delay(4, function()
    echo("{sA tied-up firestarter lieutenant exclaims {S'Wait! I know something!'{x")
    echo("{sA tied-up firestarter lieutenant says {S'That crazy guy! Uncle Carl! He would come around every week with a bag of money.'{x")
    delay(4, function()
      echo("{sA tied-up firestarter lieutenant says {S'I swear, I don't know why.'{x")
      echo("{sA tied-up firestarter lieutenant exclaims {S'Please, that's all I know. Let me go!'")
      delay(4, function()
        say("Hoss, I think that's all he knows.")
        lchar:mdo("nod")
        say("What do you want to do with him?")
        echo("You can either \"{Ckill firestarter{x\" now or \"{Csay let him go{x\".")
        lchar:setval("r10_story_progress", 11, true)
      end)
    end)
  end)
end

function indifference_speech_one()
  lchar:say("I suppose I should have expected you to lie to me. Anyone who could burn down their own city can't be trusted.")
  lchar:say("Go ahead Al, let him have it.")
  mdo("grin")
  delay(2, function()
    echo("A tied-up firestarter lieutenant screams.")
  end)
  delay(7, indifference_speech_two)
end

function indifference_speech_two()
  emote("knocks over a tied-up firestarter lieutenant's chair and starts punching him.")
  echo("A tied-up firestarter wails and sobs.")
  delay(5, indifference_speech_three)
end

function indifference_speech_three()
  echo("{sA tied-up firestarter lieutenant says {S'Wait, stop! I have something!'{x")
  echo("You nod at Al and he helps a tied-up firestarter lieutenant upright.")
  lchar:say("Go on.")
  delay(4, indifference_speech_four)
end

function indifference_speech_four()
  echo("A tied-up firestarter lieutenant looks at Al warily.")
  echo("{sA tied-up firestarter lieutenant says {S'A guy used to come by every week and drop off a bag of gold.'{x")
  echo("{sA tied-up firestarter lieutenant says {S'It was that crazy guy, Uncle Carl. I never knew why he did it.'{x")
  delay(3, function()
    say("I think that's all he knows hoss.")
    lchar:mdo("nod")
    say("What do you want to do with him?")
    echo("You can either \"{Ckill firestarter{x\" now or \"{Csay let him go{x\".")
    lchar:setval("r10_story_progress", 11, true)
  end)
end