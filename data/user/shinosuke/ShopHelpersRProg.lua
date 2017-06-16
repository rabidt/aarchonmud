-- Shop helpers.

shop={}

function shop.FindTradeItems(ch, item, itemCount)
  local found
  found = 0
  if #ch.inventory > 0 then
  	for i=1,#ch.inventory do
  	  if ch.inventory[i].vnum == item.vnum then
  	    found = found + 1
  	  end
  	end

  	if found >= itemCount then
  	  return true
  	end
  end

  return false
end

function shop.ReceiveTradeItems(ch, mob, item, itemCount)
  sendtochar(ch, "You give %s %d %s.\n", mob.shortdescr, itemCount, item.shortdescr)
  ch:echo("%s gives %s %d %s.", ch.name, mob.shortdescr, itemCount, item.shortdescr)

  for i=1,itemCount do
    ch:junk(item.name)
  end
end

function shop.HandleTrade(ch, mob, itemVnum, itemParts)
  local desiredItem = getobjproto(itemVnum)

  -- Check for the trade items.
  local partsMissing
  partsMissing = false
  for key,val in pairs(itemParts) do
    if not shop.FindTradeItems(ch, getobjproto(key), val) then
      partsMissing = true
    end
  end

  if not partsMissing then
    -- Remove trade items.
    for key,val in pairs(itemParts) do
      shop.ReceiveTradeItems(ch, mob, getobjproto(key), val)
    end

    -- Load the object and give it to the player.
    ch:oload(itemVnum)
    sendtochar(ch, "%s gives you %s.\n", mob.shortdescr, desiredItem.shortdescr)
    ch:echo("%s gives %s to %s.", mob.shortdescr, desiredItem.shortdescr, ch.name)
  else
    sendtochar(ch, "%s tells you 'You can't afford to buy %s'.\n", mob.shortdescr, desiredItem.shortdescr)
  end
end

function shop.ListItem(ch, itemVnum, itemParts)
  local itemProto 
  local partList
  itemProto = getobjproto(itemVnum)
  partList = ""

  for key,val in pairs(itemParts) do
    if not (partList == "") then
      partList = partList .. ", "
    end
    partList = partList .. val .. " " .. getobjproto(key).shortdescr
  end

  sendtochar(ch, "[ %-5d%-10s-- ] %s\n", itemProto.level, "Trade", itemProto.shortdescr)
end

function shop.ListItems(ch, items)
  sendtochar(ch, "[ Lvl  Price     Qty] Item\n")

  for key,val in pairs(items) do
    shop.ListItem(ch, key, val)
  end

  sendtochar(ch, " \n")
  sendtochar(ch, "NOTE: Buying a trade item costs materials rather than money.\n")
  sendtochar(ch, "      browse <item> to see what materials you need to buy it.\n")
  sendtochar(ch, " \n")
end

function shop.DescribeTypeDetails(mob, item)
  if item.otype == "light" then
    if item.light >= 0 then
      mob:say("It has %d hours of light remaining.", item.light)
    else
      mob:say("It is an infinite light source.")
    end
  elseif item.otype == "arrows" then
    mob:say("It contains %d arrows.", item.arrowcount)

    if item.v1 > 0 then
      mob:say("Each arrow deals %d extra %s damage.", item.arrowdamage, item.arrowdamtype)
    end
  elseif item.otype == "scroll" or item.otype == "potion" or item.otype == "pill" then
    mob:say("It has level %d spells of: %s.", item.spelllevel, table.concat(item.spells, ', '))
  elseif item.otype == "wand" or item.otype == "staff" then
    mob:say("It has %d charges of level %d '%s'.", item.chargesleft, item.spelllevel, item.spellname)
  elseif item.otype == "drinkcontainer" then
    mob:say("It holds %s-colored %s.", item.liquidcolor, item.liquid)
  elseif item.otype == "container" then
    mob:say("Capacity: %d#  Maximum weight: %d#  flags: %s", item.capacity, item.maxweight, table.concat(item:containerflag(), ", "))

    if not (item.weightmult == 100) then
      mob:say("Weight multiplier: %d%%", item.weightmult)
    end
  elseif item.otype == "weapon" then
    local typeString = string.format("a %s.", item.weapontype)

    -- Change type string for the exceptions.
    if item.weapontype == "exotic" then
      typeString = "of some exotic type."
    elseif item.weapontype == "spear" then
      typeString = "a spear or staff."
    elseif item.weapontype == "mace" then
      typeString = "a mace or club."
    elseif item.weapontype == "axe" then
      typeString = "an axe."
    end

    mob:say("This weapon is %s", typeString)
    mob:say("It does %s damage of %dd%d (average %d).", item.damnoun, item.numdice, item.dicetype, item.damavg)

    if #item:weaponflag() > 0 then
      mob:say("Weapon flags: %s", table.concat(item:weaponflag(), ", "))
    end
  elseif item.otype == "armor" then
    --wearFlags = item:wear()
    --for i=1,#wearFlags do
      --flag = wearFlags[i]
      local flag = item.weartype
      if flag == "shield" then
        mob:say("It is used as a shield.")
      elseif flag == "float" then
        mob:say("It would float nearby.")
      elseif not (flag == "take" or flag == "nosac" or flag == "translucent") then
        mob:say("It is worn on the %s.", flag)
      end
    --end

    mob:say("It provides an armor class of %d pierce, %d bash, %d slash, and %d vs. magic.", item.acpierce, item.acbash, item.acslash, item.acexotic)
  end
end

function shop.DescribeAffects(mob, item)
  -- Need access to affects tohandle this.
end

function shop.BrowseItem(ch, mob, itemName, itemList)
  local itemProto 
  local partList

  -- Trim.
  itemName = itemName:gsub("^%s*(.-)%s*$", "%1")

  if itemName == "" then
    sendtochar(ch, "Browse what?\n")
    return false
  end

  -- Try to find an item that matches.
  browsedItem = nil
  for key,val in pairs(itemList) do
    itemProto = getobjproto(key)
    if not (itemProto.name:find(itemName) == nil) then
      browsedItem = itemProto
      break
    end
  end

  if browsedItem == nil then
    return true
  end

  mob:say("Ah, an excellent choice.")
  mob:describe(browsedItem.vnum)
  
  -- Gather and report materials.
  partNames = {}
  for key,val in pairs(itemList[browsedItem.vnum]) do
    table.insert(partNames, string.format("%d %s", val, getobjproto(key).shortdescr))
  end

  mob:say("You will need %s to trade for that.", util.format_list("and", partNames))
end

function shop.CheckForTrade(ch, mob, itemName, itemList)
  local objProto
  local foundObjs

  -- Trim.
  itemName = itemName:gsub("^%s*(.-)%s*$", "%1")

  if itemName == "" then
    sendtochar(ch, "Buy what?\n")
    return false
  end

  foundObjVnum = nil
  foundObjParts = nil

  for key,val in pairs(itemList) do
    objProto = getobjproto(key)

    if not (objProto.name:lower():find(itemName:lower()) == nil) then
      foundObjVnum = key
      foundObjParts = val
      break
    end
  end

  if foundObjVnum == nil then
    return true
  end

  return shop.HandleTrade(ch, mob, foundObjVnum, foundObjParts)
end

shop.loaded = true
