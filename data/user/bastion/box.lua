-- Box room 'tag' command trigger

local BOX_VNUM = 48


local function show_tag_usage(ch)
    sendtochar(ch, [[
Syntax:
    tag keywords             - List all box numbers and their current keywords.
    tag keywords <#> <name>  - Set keywords for a box.

    tag desc                 - List all box numbers and their currend descriptions.
    tag desc <#> <desc>      - Set description for a box.

    tag reset <#>            - Reset keywords and desc for a box.
]])
end

local function box_reset(room, ch, args)
    if #args < 1 then
        show_tag_usage(ch)
        return
    end

    local box_num = tonumber(args[1])
    if nil == box_num then
        show_tag_usage(ch)
        return
    end

    local i = 1
    local box = nil
    for _,v in ipairs(room.contents) do
        if v.vnum == BOX_VNUM then
            if i == box_num then
                box = v
                break
            else
                i = i+1
            end
        end
    end

    if nil == box then
        sendtochar(ch, "There is no box #"..box_num.."\n\r")
        return
    end

    box.name = box.proto.name
    box.description = box.proto.description
    sendtochar(ch, "Box #"..box_num.." keywords and desc reset.\n\r")
end

local function box_base(propname, room, ch, args)
    if #args < 1 then
        -- List all boxes and current names
        local i = 1
        for _,v in ipairs(room.contents) do
            if v.vnum == BOX_VNUM then
                sendtochar(ch, i..": "..v[propname].."\n\r")
                i = i + 1
            end
        end

        return
    end

    if #args < 2 then
        show_tag_usage(ch)
        return
    end

    local box_num = tonumber(args[1])
    if nil == box_num then
        show_tag_usage(ch)
        return
    end

    local i = 1
    local box = nil
    for _,v in ipairs(room.contents) do
        if v.vnum == BOX_VNUM then
            if i == box_num then
                box = v
                break
            else
                i = i+1
            end
        end
    end

    if nil == box then
        sendtochar(ch, "There is no box #"..box_num.."\n\r")
        return
    end

    box[propname] = table.concat(args, " ", 2)
    sendtochar(ch, "Box #"..box_num.." "..propname.." was set to: "..box[propname].."\n\r")
end

function box_room_trigger(room, ch, text)
    local args = arguments(text, true)

    if args[1] == "keywords" then
        table.remove(args, 1)
        box_base("name", room, ch, args)
        return
    elseif args[1] == "desc" then
        table.remove(args, 1)
        box_base("description", room, ch, args)
        return
    elseif args[1] == "reset" then
        table.remove(args, 1)
        box_reset(room, ch, args)
        return
    else
        show_tag_usage(ch)
        return
    end
end
