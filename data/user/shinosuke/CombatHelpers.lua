-- VNum: 3199
-- OProg: Helpers

if Combat == nil then
    Combat = {}
end

function Combat.DamageText(damage)
    if damage < 3 then
        return "{mbothers{x"
    elseif damage < 6 then
        return "{mscratches{x"
    elseif damage < 9 then
        return "{mbruises{x"
    elseif damage < 12 then
        return "{bglances{x"
    elseif damage < 17 then
        return "{bhurts{x"
    elseif damage < 23 then
        return "{Bgrazes{x"
    elseif damage < 28 then
        return "{Bhits{x"
    elseif damage < 32 then
        return "{Binjures{x"
    elseif damage < 36 then
        return "{Cwounds{x"
    elseif damage < 41 then
        return "{CPUMMELS{x"
    elseif damage < 45 then
        return "{CMAULS{x"
    elseif damage < 50 then
        return "{GDECIMATES{x"
    elseif damage < 54 then
        return "{GDEVASTATES{x"
    elseif damage < 57 then
        return "{GMAIMS{x"
    elseif damage < 61 then
        return "{yMANGLES{x"
    elseif damage < 66 then
        return "{yDEMOLISHES{x"
    elseif damage < 75 then
        return "*** {yMUTILATES{x ***"
    elseif damage < 85 then
        return "*** {YPULVERIZES{x ***"
    elseif damage < 95 then
        return "=== {YDISMEMBERS{x ==="
    elseif damage < 105 then
        return "=== {YDISEMBOWELS{x ==="
    elseif damage < 115 then
        return ">>> {rMASSACRES{x <<<"
    elseif damage < 125 then
        return ">>> {rOBLITERATES{x <<<"
    elseif damage < 140 then
        return "{r<<< ANNIHILATES >>>{x"
    elseif damage < 160 then
        return "{r<<< DESTROYS >>>{x"
    elseif damage < 180 then
        return "{R!!! ERADICATES !!!{x"
    elseif damage < 205 then
        return "{R!!! LIQUIDATES !!!{x"
    elseif damage < 235 then
        return "{RXXX VAPORIZES XXX{x"
    elseif damage < 275 then
        return "{RXXX DISINTEGRATES XXX{x"
    elseif damage < 350 then
        return "does {+SICKENING{x damage to"
    elseif damage < 500 then
        return "does {+INSANE{x damage to"
    elseif damage < 800 then
        return "does {+UNSPEAKABLE{x things to"
    elseif damage < 1250 then
        return "does {+BLASPHEMOUS{x things to"
    elseif damage < 1500 then
        return "does {+UNBELIEVABLE{x things to"
    else
        return "does {+INCONCEIVABLE{x things to"
    end
end

function Combat.GetCharacterName(character)
    if character.ispc then
        return character.name
    else
        return character.shortdescr
    end
end

-- Prints a message like a standard hit line.
-- (string) source: The damage source. Ex: Shinosuke, Curse of Leeches, etc.
-- (CH) target: The target of the hit.
-- (int) damageDealt: The amount of damage to deal.
function Combat.DamageMessage(attacker, source, target, damageDealt)
    lethal = lethal or false

    local attackerName = Combat.GetCharacterName(attacker)
    local targetName = Combat.GetCharacterName(target)
    local damageText = Combat.DamageText(damageDealt)

    -- If damage is over a certain value we use ! instead of .
    local punctuation = "."
    if damageDealt > 30 then
        punctuation = "!"
    end

    -- Handle attacker and target same person.
    if attacker == target then
        sendtochar(attacker, "Your %s %s you%s\n", source, damageText, punctuation)
        attacker:echoaround(attacker, "%s's %s %s %sself%s", attackerName, source, damageText, attacker.himher, punctuation)
    else
        sendtochar(target, "%s's %s %s you%s\n", attackerName, source, damageText, punctuation)
        sendtochar(attacker, "Your %s %s %s%s\n", source, damageText, targetName, punctuation)
        attacker:echoaround(target, "%s's %s %s %s%s", attackerName, source, damageText, targetName, punctuation)
    end
end

-- Mostly for replacing negative damage calls with a more intuitive
-- method call when healing is intended. Defaults to not overflowing.
-- (CH) target: Character to heal.
-- (int) healAmount: How many HP to give back.
-- (bool) overflow: Whether or not healing should over flow max HP. [Optional]
function Combat.Heal(target, healAmount, overflow)
    if overflow == nil or overflow == false then
        if (target.hp + healAmount) > target.maxhp then
            healAmount = target.maxhp - target.hp
        end
    end

    target:damage(target, -healAmount, false)
end

Combat.loaded = true
