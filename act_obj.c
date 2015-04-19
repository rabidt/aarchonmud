/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *   ROM 2.4 is copyright 1993-1996 Russ Taylor             *
 *   ROM has been brought to you by the ROM consortium          *
 *       Russ Taylor (rtaylor@efn.org)                  *
 *       Gabrielle Taylor                           *
 *       Brian Moore (zump@rom.org)                     *
 *   By using this code, you have agreed to follow the terms of the     *
 *   ROM license, in the file Rom24/doc/rom.license             *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include "merc.h"
#include "tables.h"
#include "lua_scripting.h"
#include "mudconfig.h"

/* command procedures needed */
DECLARE_DO_FUN(do_split     );
DECLARE_DO_FUN(do_yell      );
DECLARE_DO_FUN(do_say       );
DECLARE_DO_FUN(do_wake      );

bool can_steal( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, bool verbose );



/*
 * Local functions.
 */
#define CD CHAR_DATA
#define OD OBJ_DATA

void      check_bomb  args( (CHAR_DATA *ch, OBJ_DATA *obj) );
void    wear_obj    args( (CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace ) );
CD *    find_keeper args( (CHAR_DATA *ch ) );
int get_cost    args( (CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy ) );
void    obj_to_keeper   args( (OBJ_DATA *obj, CHAR_DATA *ch ) );
OD *    get_obj_keeper  args( (CHAR_DATA *ch,CHAR_DATA *keeper,char *argument));
bool    expl_in_container args( ( OBJ_DATA *obj) );


#undef OD
#undef  CD

bool can_buy( CHAR_DATA *ch, OBJ_DATA *obj, bool quiet )
{
    if ( IS_NPC(ch) )
        return FALSE;

    if ( IS_IMMORTAL(ch) )
        return TRUE;

    if ( obj->clan && obj->clan != ch->clan )
    {
        if ( !quiet )
            send_to_char( "It's a clan item and you're not a member.\n\r", ch );
        return FALSE;
    }

    if ( obj->rank > ch->pcdata->clan_rank )
    {
        if ( !quiet )
            send_to_char( "Your clan rank is to low to buy this item.\n\r", ch );
        return FALSE;
    }

    return TRUE;
}

bool can_use_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( IS_NPC(ch) || IS_IMMORTAL(ch) )
        return TRUE;

    if ( obj->clan && obj->clan!=ch->clan )
        return FALSE;

    if ( !IS_NPC(ch) && obj->rank && obj->rank>ch->pcdata->clan_rank )
        return FALSE;

    /* work-around till I find this ownership de-capitalizing bug --Bobble
       if ( obj->owner != NULL && strcmp(obj->owner, ch->name) )
       return FALSE;
     */

    if ( obj->owner != NULL
            && obj->owner[0] != '\0'
            && strcmp(capitalize(obj->owner), capitalize(ch->name)) )
        return FALSE;

    return TRUE;
}

/* RT part of the corpse looting code */
/* Revamped a bit to correspond with item ownership enhancements. (Rimbol 3/98) */
bool can_loot(CHAR_DATA *ch, OBJ_DATA *obj, bool allow_group)
{
    CHAR_DATA *owner, *wch;

    if (IS_IMMORTAL(ch))
        return TRUE;

    /*
       if (obj->clan && obj->clan!=ch->clan)
       return FALSE;

       if (!IS_NPC(ch) && obj->rank && obj->rank>ch->pcdata->clan_rank)
       return FALSE;
     */

    if ( obj->owner == NULL )
        return TRUE;

    owner = NULL;
    for ( wch = char_list; wch != NULL ; wch = wch->next )
        if ( !str_cmp(capitalize(wch->name), capitalize(obj->owner)) )
            owner = wch;

    if (owner == NULL)
        return FALSE; /* Owner must be online. */

    if (!str_cmp(ch->name,owner->name))
        return TRUE;

    if ( !IS_NPC(owner) && IS_SET(owner->act,PLR_CANLOOT) )
    {
        if (obj->item_type == ITEM_CORPSE_NPC)
            return TRUE;
        if (obj->item_type == ITEM_CORPSE_PC && !is_always_safe(ch,owner))
            return TRUE;
        return FALSE;
    }

    if (allow_group && is_same_group(ch,owner))
        return TRUE;

    return FALSE;
}

/* If an object is sitting on the floor of a donation room, or is within or
   on top of another object that is on the floor of a donation room, return
   TRUE.  Otherwise (including carried objects), return FALSE. */
bool in_donation_room(OBJ_DATA *obj)
{
    if (!obj)
        return FALSE;

    if (obj->in_room)
    {
        if (IS_SET(obj->in_room->room_flags, ROOM_DONATION))
            return TRUE;
        else
            return FALSE;
    }

    if (obj->in_obj)
        if (obj->in_obj->in_room)
        {
            if (IS_SET(obj->in_obj->in_room->room_flags, ROOM_DONATION))
                return TRUE;
            else
                return FALSE;
        }

    if (obj->on)
        if (obj->on->in_room)
        {
            if (IS_SET(obj->on->in_room->room_flags, ROOM_DONATION))
                return TRUE;
            else
                return FALSE;
        }

    return FALSE;
}

void get_obj( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container )
{
    /* variables for AUTOSPLIT */
    CHAR_DATA *gch;
    int members;
    char buffer[100];

    if ( !CAN_WEAR(obj, ITEM_TAKE) )
    {
        send_to_char( "You can't take that.\n\r", ch );
        return;
    }

    if ( (!obj->in_obj || obj->in_obj->carried_by != ch)
            && (ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) ))
    {
        act( "$p: you can't carry that many items.",
                ch, obj, NULL, TO_CHAR );
        return;
    }

    if ((!obj->in_obj || obj->in_obj->carried_by != ch)
            &&  (get_carry_weight(ch) + get_obj_weight(obj) > can_carry_w(ch)))
    {
        act( "$p: you can't carry that much weight.",
                ch, obj, NULL, TO_CHAR );
        return;
    }

    if (container == NULL && !can_loot(ch,obj,FALSE))
    {
        send_to_char("You can't take that.\n\r",ch);
        return;
    }

    if (container != NULL && !can_loot(ch,container,FALSE))
    {
        send_to_char("You can't take things from that container.\n\r",ch);
        return;
    }

    if (obj->in_room != NULL)
    {
        for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
            if (gch->on == obj)
            {
                act("$N appears to be using $p.",
                        ch,obj,gch,TO_CHAR);
                return;
            }
    }

    if (in_donation_room(obj))
    {
        if (get_trust(ch) < obj->level - 2)
        {
            send_to_char("You are not powerful enough to use that.\n\r",ch);
            return;
        }
        else if (!IS_OBJ_STAT(obj, ITEM_HAD_TIMER))
            obj->timer = 0;

        REMOVE_BIT(obj->extra_flags,ITEM_HAD_TIMER);
    }

    if ( !op_percent_trigger( NULL, obj, container, ch, NULL, OTRIG_GET) )
        return;

    if ( container != NULL )
        obj_from_obj( obj );
    else
        obj_from_room( obj );

    if ( obj->item_type != ITEM_MONEY)
        obj_to_char( obj, ch );

    /* send act messages after obj_to_char to please mprog triggers --Bobble */
    if ( container != NULL )
    {
        act( "You get $p from $P.", ch, obj, container, TO_CHAR );
        act_gag( "$n gets $p from $P.", ch, obj, container, TO_ROOM, GAG_EQUIP );
    }
    else
    {
        act( "You get $p.", ch, obj, container, TO_CHAR );
        act_gag( "$n gets $p.", ch, obj, container, TO_ROOM, GAG_EQUIP );
    }

    if ( is_relic_obj(obj) )
    {
        act( "The burden of $p slows you down.", ch, obj, NULL, TO_CHAR );
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if ( IS_IMMORTAL(ch) )
        {
            logpf( "%s picked up relic %s at %d",
                    ch->name, remove_color(obj->short_descr), ch->in_room->vnum );
        }
        return;
    }

    if ( obj->item_type == ITEM_MONEY)
    {
        if ( cfg_enable_gold_mult && cfg_show_gold_mult && container && container->item_type == ITEM_CORPSE_NPC )
        {
            ptc(ch, "Creatures currently carry %d%% more gold!\n\r", (int)((cfg_gold_mult*100)-99.5));
        }
        add_money( ch, obj->value[1], obj->value[0], NULL );
        if (IS_SET(ch->act,PLR_AUTOSPLIT))
        { /* AUTOSPLIT code */
            members = 0;
            for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
            {
                if (!IS_AFFECTED(gch,AFF_CHARM) && is_same_group( gch, ch ) )
                    members++;
            }

            if ( members > 1 && (obj->value[0] > 1 || obj->value[1]))
            {
                sprintf(buffer,"%d %d",obj->value[0],obj->value[1]);
                do_split(ch,buffer);    
            }
        }

        extract_obj( obj );
    }

    return;
}



DEF_DO_FUN(do_get)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *container;
    bool found;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"from"))
        argument = one_argument(argument,arg2);

    /* Get type. */
    if ( arg1[0] == '\0' )
    {
        send_to_char( "Get what?\n\r", ch );
        return;
    }

    if ( arg2[0] == '\0' )
    {
        if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
        {
            /* 'get obj' */
            obj = get_obj_list( ch, arg1, ch->in_room->contents );
            if ( obj == NULL )
            {
                act( "I see no $T here.", ch, NULL, arg1, TO_CHAR );
                return;
            }

            get_obj( ch, obj, NULL );
        }
        else
        {
            /* 'get all' or 'get all.obj' */
            found = FALSE;
            for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
            {
                obj_next = obj->next_content;
                if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
                        &&   can_see_obj( ch, obj ) )
                {
                    found = TRUE;
                    if (IS_SET(ch->in_room->room_flags, ROOM_DONATION)
                            &&  !IS_IMMORTAL(ch))
                    {
                        send_to_char("Don't be so greedy!\n\r",ch);
                        return;
                    }
                    get_obj( ch, obj, NULL );
                }
            }

            if ( !found ) 
            {
                if ( arg1[3] == '\0' )
                    send_to_char( "I see nothing here.\n\r", ch );
                else
                    act( "I see no $T here.", ch, NULL, &arg1[4], TO_CHAR );
            }
        }
    }
    else
    {
        /* 'get ... container' */
        if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
        {
            send_to_char( "You can't do that.\n\r", ch );
            return;
        }

        if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
        {
            act( "I see no $T here.", ch, NULL, arg2, TO_CHAR );
            return;
        }

        switch ( container->item_type )
        {
            default:
                send_to_char( "That's not a container.\n\r", ch );
                return;

            case ITEM_CONTAINER:
                break;

            case ITEM_CORPSE_NPC:
            case ITEM_CORPSE_PC:
                {
                    if (!can_loot(ch,container,TRUE))
                    {
                        send_to_char("You cannot take items from that corpse.\n\r",ch);
                        return;
                    }
                }
        }

        if ( I_IS_SET(container->value[1], CONT_CLOSED) )
        {
            act( "$p is closed.", ch, container, NULL, TO_CHAR );
            return;
        }

        if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
        {
            /* 'get obj container' */
            obj = get_obj_list( ch, arg1, container->contains );
            if ( obj == NULL )
            {
                act( "I see nothing like that in $p.",
                        ch, container, NULL, TO_CHAR );
                return;
            }
            get_obj( ch, obj, container );
#ifdef BOX_LOG
            if (container->pIndexData->vnum == OBJ_VNUM_STORAGE_BOX)
            {
                char buf[MSL];
                sprintf(buf, "BOX_LOG:%s got %s (%d) from storage box.",
                        ch->name, obj->short_descr, obj->pIndexData->vnum);
                log_string(buf);
                if (obj->contains)
                {
                    OBJ_DATA *content;
                    for (content=obj->contains; content !=NULL; content=content->next_content)
                    {
                        sprintf(buf, "BOX_LOG:%s contains %s (%d).",
                                obj->short_descr,
                                content->short_descr,
                                content->pIndexData->vnum);
                        log_string(buf);
                    }
                }
            }
#endif
        }
        else
        {
            /* 'get all container' or 'get all.obj container' */
            found = FALSE;
            for ( obj = container->contains; obj != NULL; obj = obj_next )
            {
                obj_next = obj->next_content;
                if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
                        &&   can_see_obj( ch, obj ) )
                {
                    found = TRUE;
                    get_obj( ch, obj, container );
#ifdef BOX_LOG
                    if (container->pIndexData->vnum == OBJ_VNUM_STORAGE_BOX)
                    {
                        char buf[MSL];
                        sprintf(buf, "BOX_LOG:%s got %s (%d) from storage box.",
                                ch->name, obj->short_descr,
                                obj->pIndexData->vnum);
                        log_string(buf);
                        if (obj->contains)
                        {
                            OBJ_DATA *content;
                            for (content=obj->contains; content !=NULL; content=content->next_content)
                            {
                                sprintf(buf, "BOX_LOG:%s contains %s (%d).",
                                        obj->short_descr,
                                        content->short_descr,
                                        content->pIndexData->vnum);
                                log_string(buf);
                            }
                        }
                    }
#endif

                }
            }

            if ( !found )
            {
                if ( arg1[3] == '\0' )
                    act( "I see nothing in $p.",
                            ch, container, NULL, TO_CHAR );
                else
                    act( "I see nothing like that in $p.",
                            ch, container, NULL, TO_CHAR );
            }
        }
    }

    return;
}



DEF_DO_FUN(do_put)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *container;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"in") || !str_cmp(arg2,"on"))
        argument = one_argument(argument,arg2);

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Put what in what?\n\r", ch );
        return;
    }

    if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
    {
        send_to_char( "You can't do that.\n\r", ch );
        return;
    }

    if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
    {
        act( "I see no $T here.", ch, NULL, arg2, TO_CHAR );
        return;
    }

    if ( container->item_type != ITEM_CONTAINER )
    {
        send_to_char( "That's not a container.\n\r", ch );
        return;
    }

    if ( I_IS_SET(container->value[1], CONT_CLOSED) )
    {
        act( "$p is closed.", ch, container, NULL, TO_CHAR );
        return;
    }

    if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
    {
        /* 'put obj container' */
        if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
        {
            send_to_char( "You do not have that item.\n\r", ch );
            return;
        }

        if ( obj == container )
        {
            send_to_char( "You can't fold it into itself.\n\r", ch );
            return;
        }

        if ( !can_drop_obj( ch, obj ) )
        {
            send_to_char( "You can't let go of it.\n\r", ch );
            return;
        }

        /* changed by Vodur
           if (WEIGHT_MULT(obj) != 100)*/
        if ((WEIGHT_MULT(obj) != -1) /* returns -1 for non containers*/ 
                && container->pIndexData->vnum != OBJ_VNUM_STORAGE_BOX)
        {
            send_to_char("You have a feeling that would be a bad idea.\n\r",ch);
            return;
        }

        if ( get_obj_weight(obj) + get_true_weight(container)
                > (container->value[0] * 10) 
                /* v3 is capacity, should have nothing to do with weight
                   -Vodur
                   || get_obj_weight(obj) > (container->value[3] * 10)*/
                || ((get_obj_number(obj)+get_obj_number(container))
                    > container->value[3])
                || is_relic_obj(obj) )
        {
            send_to_char( "It won't fit.\n\r", ch );
            return;
        }

        if ( !op_percent_trigger( NULL, obj, container, ch, NULL, OTRIG_PUT) ) 
            return;

        obj_from_char( obj );
        obj_to_obj( obj, container );
#ifdef BOX_LOG
        if (container->pIndexData->vnum == OBJ_VNUM_STORAGE_BOX)
        {
            char buf[MSL];
            sprintf(buf, "BOX_LOG:%s put %s (%d) in storage box.",
                    ch->name, obj->short_descr, obj->pIndexData->vnum);
            log_string(buf);
            if (obj->contains)
            {
                OBJ_DATA *content;
                for (content=obj->contains; content !=NULL; content=content->next_content)
                {
                    sprintf(buf, "BOX_LOG:%s contains %s (%d).",
                            obj->short_descr,
                            content->short_descr,
                            content->pIndexData->vnum);
                    log_string(buf);
                }
            }
        }
#endif

        if (I_IS_SET(container->value[1],CONT_PUT_ON))
        {
            act("You put $p on $P.",ch,obj,container, TO_CHAR);
            act_gag("$n puts $p on $P.",ch,obj,container, TO_ROOM, GAG_EQUIP);
        }
        else
        {
            act( "You put $p in $P.", ch, obj, container, TO_CHAR );
            act_gag( "$n puts $p in $P.", ch, obj, container, TO_ROOM, GAG_EQUIP );
        }
    }
    else
    {
        /* 'put all container' or 'put all.obj container' */
        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

            if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
                    &&   can_see_obj( ch, obj )
                    /* Changed by Vodur
                       &&   WEIGHT_MULT(obj) == 100*/
                    &&   WEIGHT_MULT(obj) == -1 /*not a container*/
                    &&   obj->wear_loc == WEAR_NONE
                    &&   obj != container
                    &&   can_drop_obj( ch, obj )
                    &&   get_obj_weight( obj ) + get_true_weight( container )
                    <= (container->value[0] * 10) 
                    /* v3 is capacity, should have nothing to do with weight
                       &&   get_obj_weight(obj) < (container->value[3] * 10)*/
                    && ((get_obj_number(obj)+get_obj_number(container))
                        <= container->value[3])
                    &&   !is_relic_obj(obj) )
            {
                if ( !op_percent_trigger( NULL, obj, container, ch, NULL, OTRIG_PUT) )
                    continue;
                obj_from_char( obj );
                obj_to_obj( obj, container );
#ifdef BOX_LOG
                if (container->pIndexData->vnum == OBJ_VNUM_STORAGE_BOX)
                {
                    char buf[MSL];
                    sprintf(buf, "BOX_LOG:%s put %s (%d) in storage box.",
                            ch->name, obj->short_descr, obj->pIndexData->vnum);
                    log_string(buf);
                    if (obj->contains)
                    {
                        OBJ_DATA *content;
                        for (content=obj->contains; content !=NULL; content=content->next_content)
                        {
                            sprintf(buf, "BOX_LOG:%s contains %s (%d).",
                                    obj->short_descr,
                                    content->short_descr,
                                    content->pIndexData->vnum);
                            log_string(buf);
                        }
                    }
                }
#endif

                if (I_IS_SET(container->value[1],CONT_PUT_ON))
                {
                    act("You put $p on $P.",ch,obj,container, TO_CHAR);
                    act_gag("$n puts $p on $P.",ch,obj,container, TO_ROOM, GAG_EQUIP);
                }
                else
                {
                    act( "You put $p in $P.", ch, obj, container, TO_CHAR );
                    act_gag( "$n puts $p in $P.", ch, obj, container, TO_ROOM, GAG_EQUIP );
                }
            }

        }/*end of for loop*/
        /*let people know when it's full*/
        if (get_obj_number(container) >= container->value[3])
            printf_to_char(ch, "%s is full.\n\r", container->short_descr);
    }/*end of else (put all)*/

    return;
}



DEF_DO_FUN(do_drop)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    bool found;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Drop what?\n\r", ch );
        return;
    }

    if ( is_number( arg ) )
    {
        /* 'drop NNNN coins' */
        int amount, gold = 0, silver = 0;

        amount   = atoi(arg);
        argument = one_argument( argument, arg );
        if ( amount <= 0
                || ( str_cmp( arg, "coins" ) && str_cmp( arg, "coin" ) && 
                    str_cmp( arg, "gold"  ) && str_cmp( arg, "silver") ) )
        {
            send_to_char( "Sorry, you can't do that.\n\r", ch );
            return;
        }

        if ( !str_cmp( arg, "coins") || !str_cmp(arg,"coin") 
                ||   !str_cmp( arg, "silver"))
        {
            if (ch->silver < amount)
            {
                send_to_char("You don't have that much silver.\n\r",ch);
                return;
            }

            ch->silver -= amount;
            silver = amount;
        }

        else
        {
            if (ch->gold < amount)
            {
                send_to_char("You don't have that much gold.\n\r",ch);
                return;
            }

            ch->gold -= amount;
            gold = amount;
        }

        for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

            switch ( obj->pIndexData->vnum )
            {
                case OBJ_VNUM_SILVER_ONE:
                    silver += 1;
                    extract_obj(obj);
                    break;

                case OBJ_VNUM_GOLD_ONE:
                    gold += 1;
                    extract_obj( obj );
                    break;

                case OBJ_VNUM_SILVER_SOME:
                    silver += obj->value[0];
                    extract_obj(obj);
                    break;

                case OBJ_VNUM_GOLD_SOME:
                    gold += obj->value[1];
                    extract_obj( obj );
                    break;

                case OBJ_VNUM_COINS:
                    silver += obj->value[0];
                    gold += obj->value[1];
                    extract_obj(obj);
                    break;
            }
        }

        obj_to_room( create_money( gold, silver ), ch->in_room );
        act_gag( "$n drops some coins.", ch, NULL, NULL, TO_ROOM, GAG_EQUIP );
        send_to_char( "OK.\n\r", ch );
        return;
    }

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
    {
        /* 'drop obj' */
        if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
        {
            send_to_char( "You do not have that item.\n\r", ch );
            return;
        }

        if ( !can_drop_obj( ch, obj ) )
        {
            send_to_char( "You can't let go of it.\n\r", ch );
            return;
        }

        if ( !IS_NPC(ch) && contains_obj_recursive(obj, &is_questeq) )
        {
            send_to_char("You don't want to drop your quest equipment!\n\r", ch);
            return;
        }
        
        if (IS_SET(ch->in_room->room_flags, ROOM_DONATION))
        {
            if (obj->timer)
                SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
            else
                obj->timer = number_range(100,200);
        }

        if (!op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_DROP) )
            return;

        obj_from_char( obj );
        check_bomb (ch, obj);
        obj_to_room( obj, ch->in_room );

        act( "You drop $p.", ch, obj, NULL, TO_CHAR );
        act_gag( "$n drops $p.", ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
        {
            act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR);
            act_gag("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM,GAG_EQUIP);
            extract_obj(obj);
        }
    }
    else
    {
        /* 'drop all' or 'drop all.obj' */
        found = FALSE;
        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;

            if ( ( arg[3] == '\0' || is_name( &arg[4], obj->name ) )
                    &&   can_see_obj( ch, obj )
                    &&   obj->wear_loc == WEAR_NONE
                    &&   can_drop_obj( ch, obj ) )
            {
                found = TRUE;

                if ( !IS_NPC(ch) && contains_obj_recursive(obj, &is_questeq) )
                {
                    send_to_char("You don't want to drop your quest equipment!\n\r", ch);
                    continue;
                }
                
                if (IS_SET(ch->in_room->room_flags, ROOM_DONATION))
                {
                    if (obj->timer)
                        SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
                    else
                        obj->timer = number_range(100,200);
                }

                check_bomb(ch, obj);

                if (!op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_DROP) )
                    continue;

                obj_from_char( obj );
                obj_to_room( obj, ch->in_room );


                act( "You drop $p.", ch, obj, NULL, TO_CHAR );
                act_gag( "$n drops $p.", ch, obj, NULL, TO_ROOM, GAG_EQUIP );
                if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
                {
                    act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM);
                    act_gag("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR,GAG_EQUIP);
                    extract_obj(obj);
                }
            }
        }

        if ( !found )
        {
            if ( arg[3] == '\0' )
                act( "You are not carrying anything.",
                        ch, NULL, arg, TO_CHAR );
            else
                act( "You are not carrying any $T.",
                        ch, NULL, &arg[4], TO_CHAR );
        }
    }

    return;
}



DEF_DO_FUN(do_give)
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Give what to whom?\n\r", ch );
        return;
    }

    if ( is_number( arg1 ) )
    {
        /* 'give NNNN coins victim' */
        int amount;
        bool silver;

        amount   = atoi(arg1);
        if ( amount <= 0
                || ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) && 
                    str_cmp( arg2, "gold"  ) && str_cmp( arg2, "silver")) )
        {
            send_to_char( "Sorry, you can't do that.\n\r", ch );
            return;
        }

        silver = str_cmp(arg2,"gold");

        argument = one_argument( argument, arg2 );
        if ( arg2[0] == '\0' )
        {
            send_to_char( "Give what to whom?\n\r", ch );
            return;
        }

        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
        {
            send_to_char( "They aren't here.\n\r", ch );
            return;
        }

        if ( (!silver && ch->gold < amount) || (silver && ch->silver < amount) )
        {
            send_to_char( "You haven't got that much.\n\r", ch );
            return;
        }

        if (silver)
        {
            ch->silver      -= amount;
            add_money( victim, 0, amount, ch );
        }
        else
        {
            ch->gold        -= amount;
            add_money( victim, amount, 0, ch );
        }

        sprintf(buf,"You give $N %d %s.",amount, silver ? "silver" : "gold");
        act( buf, ch, NULL, victim, TO_CHAR    );
        sprintf(buf,"$n gives you %d %s.",amount, silver ? "silver" : "gold");
        act( buf, ch, NULL, victim, TO_VICT    );
        act( "$n gives $N some coins.",  ch, NULL, victim, TO_NOTVICT );

        /*
         * Bribe trigger
         */
        if ( IS_NPC(victim) && HAS_TRIGGER( victim, TRIG_BRIBE ) )
            mp_bribe_trigger( victim, ch, silver ? amount : amount * 100 );


        if (IS_NPC(victim) && IS_SET(victim->act,ACT_IS_CHANGER))
        {
            int change;

            change = (silver ? 95 * amount / 100 / 100 
                    : 95 * amount);


            if (!silver && change > victim->silver)
                victim->silver += change;

            if (silver && change > victim->gold)
                victim->gold += change;

            if (!silver && (get_carry_weight(ch) +
                        change/100 > can_carry_w(ch) ))
            {
                send_to_char("You can't carry that much silver.\n\r", ch);
                sprintf(buf,"%d gold %s", amount, ch->name);
                do_give(victim,buf);
                return;
            }

            else if (change < 1 && can_see(victim,ch))
            {
                act("{t$n tells you {T'I'm sorry, I cannot change that amount for you.'{x",victim,NULL,ch,TO_VICT);
                //ch->reply = victim;
                sprintf(buf,"%d %s %s", amount, silver ? "silver" : "gold",ch->name);
                do_give(victim,buf);
            }
            else if (can_see(victim,ch))
            {
                sprintf(buf,"%d %s %s", change, silver ? "gold" : "silver",ch->name);
                do_give(victim,buf);
                if (silver)
                {
                    sprintf(buf,"%d silver %s", (95 * amount / 100 - change * 100),ch->name);
                    do_give(victim,buf);
                }
                act("{t$n tells you {T'Thank you, come again.'{x", victim,NULL,ch,TO_VICT);
                //ch->reply = victim;
            }
            else
            {
                do_say( victim, "I don't trade with folks I can't see.  I'll just drop your money here." );
                sprintf(buf,"%d %s", amount, silver ? "silver" : "gold");
                do_drop(victim,buf);
            }
        }
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
        send_to_char( "You do not have that item.\n\r", ch );
        return;
    }

    if ( obj->wear_loc != WEAR_NONE )
    {
        send_to_char( "You must remove it first.\n\r", ch );
        return;
    }

    if ( obj->item_type == ITEM_EXPLOSIVE && obj->timer > 0 )
    {
        send_to_char( "You cannot give away lit explosives!\n\r", ch);
        return;
    }

    if ( is_relic_obj(obj) )
    {
        send_to_char( "You cannot give away relics!\n\r", ch);
        return;
    }

    if ( obj->item_type == ITEM_CONTAINER )
        if (expl_in_container(obj))
        {
            send_to_char( "That container has a lit explosive inside!  They don't want it.\n\r",ch);
            return;
        }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
    {
        act("$N tells you 'Sorry, you'll have to sell that.'",
                ch,NULL,victim,TO_CHAR);
        ch->reply = victim;
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
        send_to_char( "You can't let go of it.\n\r", ch );
        return;
    }

    if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_NOACCEPT)
            && !(IS_NPC(ch) || IS_IMMORTAL(ch)) )
    {
        act( "$N won't accept items from you.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( victim->carry_number + get_obj_number( obj ) > can_carry_n( victim ) )
    {
        act( "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if (get_carry_weight(victim) + get_obj_weight(obj) > can_carry_w( victim ) )
    {
        act( "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( !can_see_obj( victim, obj ) )
    {
        act( "$N can't see it.", ch, NULL, victim, TO_CHAR );
        return;
    }
    
    if ( IS_NPC(victim) && contains_obj_recursive(obj, &is_questeq) )
    {
        // we allow giving quest eq to mobs specifically designed for this
        if ( has_mp_trigger_vnum(victim, TRIG_GIVE, obj->pIndexData->vnum) )
        {
            logpf("%s giving quest item #%d to mob #%d at room #%d",
                ch->name, obj->pIndexData->vnum, victim->pIndexData->vnum, ch->in_room->vnum
            );
        }
        else
        {
            send_to_char("You don't want to give away your quest equipment!\n\r", ch);
            return;
        }
    }

    /* oprog check */
    if (!op_percent_trigger( NULL, obj, NULL, ch, victim, OTRIG_GIVE) )
        return;

    obj_from_char( obj );
    obj_to_char( obj, victim );
    act( "You give $p to $N.", ch, obj, victim, TO_CHAR    );
    act( "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT );
    act( "$n gives you $p.",   ch, obj, victim, TO_VICT    );

    /*
     * Give trigger
     */
    bool give_trigger_activated = FALSE;
    if ( IS_NPC(victim) && HAS_TRIGGER( victim, TRIG_GIVE ) )
        give_trigger_activated = mp_give_trigger( victim, ch, obj );
    // NPCs typically don't want items, so we drop them to prevent lots of possible screw-ups
    // where players give the wrong items to the wrong NPCs
    if ( !give_trigger_activated 
            && !is_mprog_running() && !g_LuaScriptInProgress
            && !HAS_OTRIG( obj, OTRIG_GIVE ) 
            && IS_NPC(victim) 
            && !IS_AFFECTED(victim, AFF_CHARM) )
    {
        act( "$n shrugs and drops $p.", victim, obj, NULL, TO_ROOM );
        obj_from_char( obj );
        obj_to_room( obj, victim->in_room );
    }

    /* imms giving stuff to alts.. */
    /* if ( IS_IMMORTAL(ch) && !IS_NPC(victim) && is_same_player(ch, victim) ) */
    if ( IS_IMMORTAL(ch) && !IS_NPC(victim))
        logpf( "do_give: %s giving obj %d to %s",
                ch->name, obj->pIndexData->vnum, victim->name );

    return;
}

int flag_add_malus( OBJ_DATA *weapon )
{
    int flag, nr = 0;
    /* count flags */
    for ( flag = 0; weapon_type2[flag].name != NULL; flag++ )
        if ( IS_WEAPON_STAT(weapon, weapon_type2[flag].bit) )
            nr++;
    /* twohand weapons can take more flags */
    if ( IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS) )
        return 8 * (nr - 1);
    else
        return 10 * nr;
}

/* for poisoning weapons and food/drink */
DEF_DO_FUN(do_envenom)
{
    OBJ_DATA *obj;
    AFFECT_DATA af; 
    int percent,skill;

    /* find out what */
    if (argument[0] == '\0')
    {
        send_to_char("Envenom what item?\n\r",ch);
        return;
    }

    obj =  get_obj_list(ch,argument,ch->carrying);

    if (obj== NULL)
    {
        send_to_char("You don't have that item.\n\r",ch);
        return;
    }

    if ((skill = get_skill(ch,gsn_envenom)) < 1)
    {
        send_to_char("Are you crazy? You'd poison yourself!\n\r",ch);
        return;
    }

    if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
    {
        if (IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
        {
            act("You fail to poison $p.",ch,obj,NULL,TO_CHAR);
            return;
        }

        if (number_percent() < skill)  /* success! */
        {
            act("You treat $p with deadly poison.",ch,obj,NULL,TO_CHAR);
            act("$n treats $p with deadly poison.",ch,obj,NULL,TO_ROOM);
            if (!obj->value[3])
            {
                obj->value[3] = 1;
                check_improve(ch,gsn_envenom,TRUE,3);
            }
            WAIT_STATE(ch,skill_table[gsn_envenom].beats);
            return;
        }

        act("You fail to poison $p.",ch,obj,NULL,TO_CHAR);
        if (!obj->value[3])
            check_improve(ch,gsn_envenom,FALSE,3);
        WAIT_STATE(ch,skill_table[gsn_envenom].beats);
        return;
    }

    if (obj->item_type == ITEM_WEAPON)
    {

        if ( is_ranged_weapon(obj) )
        {
            send_to_char("You can only envenom melee weapons.\n\r",ch);
            return;
        }        

        if (obj->value[0] < 0 || (weapon_base_damage[obj->value[0]] == DAM_BASH))
        {
            send_to_char("You can only envenom edged weapons.\n\r",ch);
            return;
        }

        if (IS_WEAPON_STAT(obj,WEAPON_POISON) || IS_WEAPON_STAT(obj,WEAPON_PARALYSIS_POISON))
        {
            act("$p is already poisoned.",ch,obj,NULL,TO_CHAR);
            return;
        }

        percent = number_percent();
        if ( percent < skill - flag_add_malus(obj) )
        {

            af.where     = TO_WEAPON;
            af.type      = gsn_poison;
            af.level     = ch->level;
            af.duration  = get_duration(gsn_envenom, ch->level);
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj(obj,&af);

            act("You coat $p with venom.",ch,obj,NULL,TO_CHAR);
            act("$n coats $p with deadly venom.",ch,obj,NULL,TO_ROOM);
            check_improve(ch,gsn_envenom,TRUE,3);
            WAIT_STATE(ch,skill_table[gsn_envenom].beats);
            return;
        }
        else
        {
            act("You fail to envenom $p.",ch,obj,NULL,TO_CHAR);
            check_improve(ch,gsn_envenom,FALSE,3);
            WAIT_STATE(ch,skill_table[gsn_envenom].beats);
            return;
        }
    }

    act("You can't poison $p.",ch,obj,NULL,TO_CHAR);
    return;
}

DEF_DO_FUN(do_paralysis_poison)
{
    OBJ_DATA *obj;
    AFFECT_DATA af; 
    int percent,skill;

    if (argument[0] == '\0')
    {
        send_to_char("Coat which weapon with paralysis poison?\n\r",ch);
        return;
    }

    obj =  get_obj_list(ch,argument,ch->carrying);

    if (obj== NULL)
    {
        send_to_char("You don't have that item.\n\r",ch);
        return;
    }

    if ((skill = get_skill(ch,gsn_paralysis_poison)) < 1)
    {
        send_to_char("Are you crazy? You'd poison yourself!\n\r",ch);
        return;
    }

    if (obj->item_type == ITEM_WEAPON)
    {

        if (IS_WEAPON_STAT(obj,WEAPON_POISON) || IS_WEAPON_STAT(obj,WEAPON_PARALYSIS_POISON))
        {
            act("$p is already poisoned.",ch,obj,NULL,TO_CHAR);
            return;
        }

        percent = number_percent();
        if ( percent < skill - flag_add_malus(obj) )
        {
            af.where     = TO_WEAPON;
            af.type      = gsn_poison;
            af.level     = ch->level;
            af.duration  = get_duration(gsn_paralysis_poison, ch->level);
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_PARALYSIS_POISON;
            affect_to_obj(obj,&af);
            act("You coat $p with a paralysis poison.",ch,obj,NULL,TO_CHAR);
            act("$n coats $p with a paralysis poison.",ch,obj,NULL,TO_ROOM);
            check_improve(ch,gsn_paralysis_poison,TRUE,3);
            WAIT_STATE(ch,skill_table[gsn_paralysis_poison].beats);
            return;
        }
        else
        {
            act("You fail to poison $p.",ch,obj,NULL,TO_CHAR);
            check_improve(ch,gsn_paralysis_poison,FALSE,3);
            WAIT_STATE(ch,skill_table[gsn_paralysis_poison].beats);
            return;
        }
    }

    act("You can't poison $p.",ch,obj,NULL,TO_CHAR);
    return;
}

DEF_DO_FUN(do_fill)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *fountain;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Fill what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        send_to_char( "You do not have that item.\n\r", ch );
        return;
    }

    fountain = get_obj_by_type( ch->in_room->contents, ITEM_FOUNTAIN );

    if ( fountain == NULL )
    {
        send_to_char( "There is no fountain here!\n\r", ch );
        return;
    }

    if ( obj->item_type != ITEM_DRINK_CON )
    {
        send_to_char( "You can't fill that.\n\r", ch );
        return;
    }

    if ( obj->value[1] != 0 && obj->value[2] != fountain->value[2] )
    {
        send_to_char( "There is already another liquid in it.\n\r", ch );
        return;
    }

    if ( obj->value[1] >= obj->value[0] )
    {
        send_to_char( "Your container is full.\n\r", ch );
        return;
    }

    sprintf(buf,"You fill $p with %s from $P.",
            liq_table[fountain->value[2]].liq_name);
    act( buf, ch, obj,fountain, TO_CHAR );
    sprintf(buf,"$n fills $p with %s from $P.",
            liq_table[fountain->value[2]].liq_name);
    act(buf,ch,obj,fountain,TO_ROOM);
    obj->value[2] = fountain->value[2];
    obj->value[1] = obj->value[0];
    return;
}

DEF_DO_FUN(do_pour)
{
    char arg[MAX_STRING_LENGTH],buf[MAX_STRING_LENGTH];
    OBJ_DATA *out, *in;
    CHAR_DATA *vch = NULL;
    int amount;

    argument = one_argument(argument,arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char("Pour what into what?\n\r",ch);
        return;
    }


    if ((out = get_obj_carry(ch,arg, ch)) == NULL)
    {
        send_to_char("You don't have that item.\n\r",ch);
        return;
    }

    if (out->item_type != ITEM_DRINK_CON)
    {
        send_to_char("That's not a drink container.\n\r",ch);
        return;
    }

    if (!str_cmp(argument,"out"))
    {
        if (out->value[1] == 0)
        {
            send_to_char("It's already empty.\n\r",ch);
            return;
        }

        out->value[1] = 0;
        out->value[3] = 0;
        sprintf(buf,"You invert $p, spilling %s all over the ground.",
                liq_table[out->value[2]].liq_name);
        act(buf,ch,out,NULL,TO_CHAR);

        sprintf(buf,"$n inverts $p, spilling %s all over the ground.",
                liq_table[out->value[2]].liq_name);
        act(buf,ch,out,NULL,TO_ROOM);
        return;
    }

    if ((in = get_obj_here(ch,argument)) == NULL)
    {
        vch = get_char_room(ch,argument);

        if (vch == NULL)
        {
            send_to_char("Pour into what?\n\r",ch);
            return;
        }

        in = get_eq_char(vch,WEAR_HOLD);

        if (in == NULL)
        {
            send_to_char("They aren't holding anything.",ch);
            return;
        }
    }

    if (in->item_type != ITEM_DRINK_CON)
    {
        send_to_char("You can only pour into other drink containers.\n\r",ch);
        return;
    }

    if (in == out)
    {
        send_to_char("You cannot change the laws of physics!\n\r",ch);
        return;
    }

    if (in->value[1] != 0 && in->value[2] != out->value[2])
    {
        send_to_char("They don't hold the same liquid.\n\r",ch);
        return;
    }

    if (out->value[1] == 0)
    {
        act("There's nothing in $p to pour.",ch,out,NULL,TO_CHAR);
        return;
    }

    if (in->value[1] >= in->value[0])
    {
        act("$p is already filled to the top.",ch,in,NULL,TO_CHAR);
        return;
    }

    amount = UMIN(out->value[1],in->value[0] - in->value[1]);

    in->value[1] += amount;
    out->value[1] -= amount;
    in->value[2] = out->value[2];

    if (vch == NULL)
    {
        sprintf(buf,"You pour %s from $p into $P.",
                liq_table[out->value[2]].liq_name);
        act(buf,ch,out,in,TO_CHAR);
        sprintf(buf,"$n pours %s from $p into $P.",
                liq_table[out->value[2]].liq_name);
        act(buf,ch,out,in,TO_ROOM);
    }
    else
    {
        sprintf(buf,"You pour some %s for $N.",
                liq_table[out->value[2]].liq_name);
        act(buf,ch,NULL,vch,TO_CHAR);
        sprintf(buf,"$n pours you some %s.",
                liq_table[out->value[2]].liq_name);
        act(buf,ch,NULL,vch,TO_VICT);
        sprintf(buf,"$n pours some %s for $N.",
                liq_table[out->value[2]].liq_name);
        act(buf,ch,NULL,vch,TO_NOTVICT);

    }
}

DEF_DO_FUN(do_drink)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int amount;
    int liquid;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        obj = get_obj_by_type( ch->in_room->contents, ITEM_FOUNTAIN );
        if ( obj == NULL )
        {
            send_to_char( "Drink what?\n\r", ch );
            return;
        }
    }
    else
    {
        if ( ( obj = get_obj_here( ch, arg ) ) == NULL )
        {
            send_to_char( "You can't find it.\n\r", ch );
            return;
        }
    }

    if ( !can_use_obj(ch, obj) )
    {
        send_to_char("You can't use that.\n\r",ch);
        return;
    }

    if (obj->level > (ch->level + 5) )
    {
        send_to_char("You spill it all over yourself.\n\r",ch);
        obj->value[1] -= UMIN(3,obj->value[1]);
        return;
    }

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10 )
    {
        send_to_char( "You fail to reach your mouth.  *Hic*\n\r", ch );
        return;
    }

    switch ( obj->item_type )
    {
        default:
            send_to_char( "You can't drink from that.\n\r", ch );
            return;

        case ITEM_FOUNTAIN:
            if ( ( liquid = obj->value[2] )  < 0 )
            {
                bug( "Do_drink: bad liquid number %d.", liquid );
                liquid = obj->value[2] = 0;
            }
            amount = liq_table[liquid].liq_affect[4] * 3;
            break;

        case ITEM_DRINK_CON:
            if ( obj->value[1] <= 0 )
            {
                send_to_char( "It is already empty.\n\r", ch );
                return;
            }

            if ( ( liquid = obj->value[2] )  < 0 )
            {
                bug( "Do_drink: bad liquid number %d.", liquid );
                liquid = obj->value[2] = 0;
            }

            amount = liq_table[liquid].liq_affect[4];
            amount = UMIN(amount, obj->value[1]);
            break;
    }
    if (!IS_NPC(ch) && !IS_IMMORTAL(ch) 
            &&  ch->pcdata->condition[COND_FULL] > 65
            &&  ch->pcdata->condition[COND_THIRST] > 30)
    {
        send_to_char("You're too full to drink more.\n\r",ch);
        return;
    }

    if ( !op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_DRINK) )
        return;

    act( "You drink $T from $p.",
            ch, obj, liq_table[liquid].liq_name, TO_CHAR );
    act( "$n drinks $T from $p.",
            ch, obj, liq_table[liquid].liq_name, TO_ROOM );

    gain_condition( ch, COND_DRUNK,
            amount * liq_table[liquid].liq_affect[COND_DRUNK] / 36 );
    gain_condition( ch, COND_FULL,
            amount * liq_table[liquid].liq_affect[COND_FULL] / 4 );
    gain_condition( ch, COND_THIRST,
            amount * liq_table[liquid].liq_affect[COND_THIRST] / 10 );
    gain_condition(ch, COND_HUNGER,
            amount * liq_table[liquid].liq_affect[COND_HUNGER] / 2 );

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]  > 10 )
        send_to_char( "You feel drunk.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL]   > 60 )
        send_to_char( "You are full.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 60 )
        send_to_char( "Your thirst is quenched.\n\r", ch );

    if (obj->value[0] > 0)
        obj->value[1] -= amount;

    if ( obj->value[3] != 0 )
    {
        /* The drink was poisoned ! */
        AFFECT_DATA af;

        af.where     = TO_AFFECTS;
        af.type      = gsn_poison;
        af.level     = number_fuzzy(amount); 
        af.duration  = 3 * amount;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_POISON;
        affect_join( ch, &af );
        send_to_char( "You choke and gag.\n\r", ch );
        act( "$n chokes and gags.", ch, NULL, NULL, TO_ROOM );
    }

    return;
}



DEF_DO_FUN(do_eat)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Eat what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        send_to_char( "You do not have that item.\n\r", ch );
        return;
    }

    /* Added a check so that immortals can eat anything - Astark 12-23-12 */
    if (!IS_IMMORTAL(ch))
    {
        if (obj->level>ch->level+5)
        {
            send_to_char("Its too hard to swallow.\n\r",ch);
            return; 
        }
    }

    if ( !IS_IMMORTAL(ch) )
    {
        if ( obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL )
        {
            send_to_char( "That's not edible.\n\r", ch );
            return;
        }

        if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 65
                && obj->item_type == ITEM_FOOD &&
                ch->pcdata->condition[COND_HUNGER] > 30)
        {   
            send_to_char( "You are too full to eat more.\n\r", ch );
            return;
        }
    }

    if ( !can_use_obj(ch, obj) )
    {
        send_to_char("You can't eat that.\n\r",ch);
        return;
    }

    if ( !op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_EAT) )
        return;

    act( "You eat $p.", ch, obj, NULL, TO_CHAR );
    act( "$n eats $p.",  ch, obj, NULL, TO_ROOM );

    switch ( obj->item_type )
    {

        case ITEM_FOOD:
            if ( !IS_NPC(ch) )
            {
                int condition;

                condition = ch->pcdata->condition[COND_HUNGER];
                gain_condition( ch, COND_FULL, obj->value[0] );
                gain_condition( ch, COND_HUNGER, obj->value[1]);
                if ( condition>=0 && condition < 20 && ch->pcdata->condition[COND_HUNGER] >= 20 )
                    send_to_char( "You are no longer hungry.\n\r", ch );
                else if ( ch->pcdata->condition[COND_FULL] > 60 )
                    send_to_char( "You are full.\n\r", ch );
            }

            if ( obj->value[3] != 0 )
            {
                /* The food was poisoned! */
                AFFECT_DATA af;

                af.where     = TO_AFFECTS;
                af.type      = gsn_poison;
                af.level     = number_fuzzy(obj->value[0]);
                af.duration  = 2 * obj->value[0];
                af.location  = APPLY_NONE;
                af.modifier  = 0;
                af.bitvector = AFF_POISON;
                affect_join( ch, &af );

                send_to_char( "You choke and gag.\n\r", ch );
                act( "$n chokes and gags.", ch, 0, 0, TO_ROOM );
            }
            break;

        case ITEM_PILL:
            obj_cast_spell( obj->value[1], obj->value[0], ch, obj, "self", FALSE );
            obj_cast_spell( obj->value[2], obj->value[0], ch, obj, "self", FALSE );
            obj_cast_spell( obj->value[3], obj->value[0], ch, obj, "self", FALSE );
            obj_cast_spell( obj->value[4], obj->value[0], ch, obj, "self", FALSE );
            break;
    }

    extract_obj( obj );
    return;
}

/*
 * Remove an object.
 */
bool remove_obj( CHAR_DATA *ch, int iWear, bool fReplace )
{
    OBJ_DATA *obj;

    if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
        return TRUE;

    if ( !fReplace )
        return FALSE;

    if ( IS_SET(obj->extra_flags, ITEM_NOREMOVE) && !IS_IMMORTAL(ch) )
    {
        act( "You can't remove $p.", ch, obj, NULL, TO_CHAR );
        return FALSE;
    }

    unequip_char( ch, obj );
    act( "You stop using $p.", ch, obj, NULL, TO_CHAR );
    act_gag( "$n stops using $p.", ch, obj, NULL, TO_ROOM, GAG_EQUIP );
    return TRUE;
}

/* remove obj for new obj when 2 locations are possible --Bobble */
bool remove_smart( CHAR_DATA *ch, OBJ_DATA *new_obj, int iWear1, int iWear2, bool fReplace )
{
    OBJ_DATA *obj1, *obj2;
    int vnum;

    if ( new_obj == NULL )
    {
        bugf( "remove_smart: new_obj == NULL" );
        return FALSE;
    }

    vnum = new_obj->pIndexData->vnum;
    obj1 = get_eq_char( ch, iWear1 );
    obj2 = get_eq_char( ch, iWear2 );

    if ( obj1 == NULL || obj2 == NULL )
        return TRUE;

    if ( obj1->pIndexData->vnum == vnum && obj2->pIndexData->vnum != vnum )
    {
        return remove_obj( ch, iWear2, fReplace )
            || remove_obj( ch, iWear1, fReplace );
    }
    else
    {
        return remove_obj( ch, iWear1, fReplace )
            || remove_obj( ch, iWear2, fReplace );
    }
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *weapon=NULL;
    int wear_level;

    if ( !can_use_obj(ch, obj) ) /* Check ownership. */
    {
        send_to_char("You can't wear that.\n\r",ch);
        return;
    }

    wear_level = ch->level;
    if ( IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM) )
        wear_level = UMIN( LEVEL_HERO, wear_level );
    if ( wear_level < obj->level )
    {
        sprintf( buf, "You must be level %d to use this object.\n\r",
                obj->level );
        send_to_char( buf, ch );
        act_gag( "$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        return;
    }

    if ( obj->item_type == ITEM_LIGHT )
    {
        if ( !remove_obj( ch, WEAR_LIGHT, fReplace ) )
            return;
        act( "You light $p and hold it.",  ch, obj, NULL, TO_CHAR );
        act_gag( "$n lights $p and holds it.", ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        equip_char( ch, obj, WEAR_LIGHT );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FINGER ) )
    {
        if ( !remove_smart(ch, obj, WEAR_FINGER_L, WEAR_FINGER_R, fReplace) )
            return;

        if ( get_eq_char( ch, WEAR_FINGER_L ) == NULL )
        {
            act( "You wear $p on your left finger.",  ch, obj, NULL, TO_CHAR );
            act_gag( "$n wears $p on $s left finger.", ch, obj, NULL, TO_ROOM, GAG_EQUIP );
            equip_char( ch, obj, WEAR_FINGER_L );
            return;
        }

        if ( get_eq_char( ch, WEAR_FINGER_R ) == NULL )
        {
            act( "You wear $p on your right finger.", ch, obj, NULL, TO_CHAR );
            act_gag( "$n wears $p on $s right finger.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
            equip_char( ch, obj, WEAR_FINGER_R );
            return;
        }

        bug( "Wear_obj: no free finger.", 0 );
        send_to_char( "You already wear two rings.\n\r", ch );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_NECK ) )
    {
        if ( !remove_smart(ch, obj, WEAR_NECK_1, WEAR_NECK_2, fReplace) )
            return;

        if ( get_eq_char( ch, WEAR_NECK_1 ) == NULL )
        {
            act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            act_gag( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
            equip_char( ch, obj, WEAR_NECK_1 );
            return;
        }

        if ( get_eq_char( ch, WEAR_NECK_2 ) == NULL )
        {
            act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            act_gag( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
            equip_char( ch, obj, WEAR_NECK_2 );
            return;
        }

        bug( "Wear_obj: no free neck.", 0 );
        send_to_char( "You already wear two neck items.\n\r", ch );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_TORSO ) )
    {
        if ( !remove_obj( ch, WEAR_TORSO, fReplace ) )
            return;
        act( "You wear $p on your torso.", ch, obj, NULL, TO_CHAR );
        act_gag( "$n wears $p on $s torso.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        equip_char( ch, obj, WEAR_TORSO );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HEAD ) )
    {
        if ( !remove_obj( ch, WEAR_HEAD, fReplace ) )
            return;
        act_gag( "$n wears $p on $s head.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You wear $p on your head.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_HEAD );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_LEGS ) )
    {
        if ( !remove_obj( ch, WEAR_LEGS, fReplace ) )
            return;
        act_gag( "$n wears $p on $s legs.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You wear $p on your legs.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_LEGS );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FEET ) )
    {
        if ( !remove_obj( ch, WEAR_FEET, fReplace ) )
            return;
        act_gag( "$n wears $p on $s feet.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You wear $p on your feet.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_FEET );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HANDS ) )
    {
        if ( !remove_obj( ch, WEAR_HANDS, fReplace ) )
            return;
        act_gag( "$n wears $p on $s hands.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You wear $p on your hands.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_HANDS );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ARMS ) )
    {
        if ( !remove_obj( ch, WEAR_ARMS, fReplace ) )
            return;
        act_gag( "$n wears $p on $s arms.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You wear $p on your arms.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_ARMS );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ABOUT ) )
    {
        if ( !remove_obj( ch, WEAR_ABOUT, fReplace ) )
            return;
        act_gag( "$n wears $p about $s body.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You wear $p about your body.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_ABOUT );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WAIST ) )
    {
        if ( !remove_obj( ch, WEAR_WAIST, fReplace ) )
            return;
        act_gag( "$n wears $p about $s waist.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You wear $p about your waist.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_WAIST );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WRIST ) )
    {
        if ( !remove_smart(ch, obj, WEAR_WRIST_L, WEAR_WRIST_R, fReplace) )
            return;

        if ( get_eq_char( ch, WEAR_WRIST_L ) == NULL )
        {
            act_gag( "$n wears $p around $s left wrist.",
                    ch, obj, NULL, TO_ROOM, GAG_EQUIP );
            act( "You wear $p around your left wrist.",
                    ch, obj, NULL, TO_CHAR );
            equip_char( ch, obj, WEAR_WRIST_L );
            return;
        }

        if ( get_eq_char( ch, WEAR_WRIST_R ) == NULL )
        {
            act_gag( "$n wears $p around $s right wrist.",
                    ch, obj, NULL, TO_ROOM, GAG_EQUIP );
            act( "You wear $p around your right wrist.",
                    ch, obj, NULL, TO_CHAR );
            equip_char( ch, obj, WEAR_WRIST_R );
            return;
        }

        bug( "Wear_obj: no free wrist.", 0 );
        send_to_char( "You already wear two wrist items.\n\r", ch );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_SHIELD ) )
    {
        if ( !remove_obj( ch, WEAR_SHIELD, fReplace ) )
            return;
        act_gag( "$n wears $p as a shield.", ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You wear $p as a shield.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_SHIELD );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_HOLD ) )
    {
        if (get_eq_char (ch, WEAR_SECONDARY) != NULL)
        {
            send_to_char ("You cannot hold an item while using 2 weapons.\n\r",ch);
            return;
        }

        if (((weapon = get_eq_char(ch,WEAR_WIELD)) != NULL) &&
                IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS))
        {
            send_to_char( "You cannot hold an item while using a two-handed weapon.\n\r", ch );
            return;
        }

        if ( !remove_obj( ch, WEAR_HOLD, fReplace ) )
            return;
        act_gag( "$n holds $p in $s hands.",   ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You hold $p in your hands.", ch, obj, NULL, TO_CHAR );
        equip_char( ch, obj, WEAR_HOLD );
        return;
    }

    if ( CAN_WEAR( obj, ITEM_WIELD ) )
    {
        int sn,skill;

        if ( !remove_obj( ch, WEAR_WIELD, fReplace ) )
            return;

        if (get_eq_char(ch, WEAR_WIELD)!= NULL)
        {
            if ( !remove_obj( ch, WEAR_WIELD, fReplace ) )
                return;
        }

        if ( !IS_NPC(ch) 
                && get_obj_weight(obj) > (ch_str_wield(ch)))
        {
            send_to_char( "It is too heavy for you to wield.\n\r", ch );
            return;
        }

        if ( !IS_NPC(ch) && IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS) && get_eq_char(ch, WEAR_HOLD) != NULL )
        {
            send_to_char("You need two hands free for that weapon.\n\r",ch);
            return;
        }


        act_gag( "$n wields $p.", ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        act( "You wield $p.", ch, obj, NULL, TO_CHAR );

        equip_char( ch, obj, WEAR_WIELD );

        sn = get_weapon_sn(ch);

        if (sn == gsn_hand_to_hand)
            return;

        skill = get_weapon_skill(ch,sn);

        if (skill >= 100)
            act("You are supremely talented with $p.",ch,obj,NULL,TO_CHAR);
        else if (skill > 90)
            act("$p seems like an extension of your body.",ch,obj,NULL,TO_CHAR);
        else if (skill > 80)
            act("You feel quite confident with $p.",ch,obj,NULL,TO_CHAR);
        else if (skill > 74)
            act("You are skilled with $p.",ch,obj,NULL,TO_CHAR);
        else if (skill > 60)
            act("Your skill with $p is adequate.",ch,obj,NULL,TO_CHAR);
        else if (skill > 35)
            act("$p feels a little clumsy in your hands.",ch,obj,NULL,TO_CHAR);
        else if (skill > 1)
            act("You fumble and almost drop $p.",ch,obj,NULL,TO_CHAR);
        else
            act("You don't even know which end is up on $p.",
                    ch,obj,NULL,TO_CHAR);

        return;
    }

    if ( CAN_WEAR(obj,ITEM_WEAR_FLOAT) )
    {
        if (!remove_obj(ch,WEAR_FLOAT, fReplace) )
            return;
        act_gag("$n releases $p to float next to $m.",ch,obj,NULL,TO_ROOM,GAG_EQUIP);
        act("You release $p and it floats next to you.",ch,obj,NULL,TO_CHAR);
        equip_char(ch,obj,WEAR_FLOAT);
        return;
    }

    if ( fReplace )
        send_to_char( "You can't wear, wield, or hold that.\n\r", ch );

    return;
}


DEF_DO_FUN(do_wear)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Wear, wield, or hold what?\n\r", ch );
        return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
        OBJ_DATA *obj_next;

        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
                if (op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_WEAR) )
                    wear_obj( ch, obj, FALSE );
        }
        return;
    }
    else
    {
        if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
        {
            send_to_char( "You do not have that item.\n\r", ch );
            return;
        }

        if (op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_WEAR) )
            wear_obj( ch, obj, TRUE );
        else
            return;
    }

    return;
}


DEF_DO_FUN(do_remove)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Remove what?\n\r", ch );
        return;
    }
    if ( !str_cmp( arg, "all" ) )
    {
        OBJ_DATA *obj_next;
        for ( obj = ch->carrying; obj != NULL; obj = obj_next )
        {
            obj_next = obj->next_content;
            if(obj->wear_loc != WEAR_NONE) 
                if (op_percent_trigger(NULL, obj, NULL, ch, NULL, OTRIG_REMOVE) )
                    remove_obj(ch,obj->wear_loc, TRUE);
        }
        return;
    }
    else
    {
        if ( ( obj = get_obj_wear( ch, arg ) ) == NULL )
        {
            send_to_char( "You do not have that item.\n\r", ch );
            return;
        }
        if (op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_REMOVE) )
            remove_obj( ch, obj->wear_loc, TRUE );
        return;
    }
}

int get_obj_faith_worth( OBJ_DATA *obj )
{
    int worth = 0;
    OBJ_DATA *in_obj;

    if ( obj == NULL )
        return 0;

    if ( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
        worth = obj->level;
    else
        worth = obj->cost/100;
    /* add worth of content */
    for ( in_obj = obj->contains; in_obj != NULL; in_obj = in_obj->next_content )
        worth += get_obj_faith_worth( in_obj );

    return worth;
}

DEF_DO_FUN(do_sacrifice)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int silver, worth;
    const char *god_name;

    /* variables for AUTOSPLIT */
    CHAR_DATA *gch;
    int members;
    char buffer[100];

    one_argument( argument, arg );


    if ((obj = get_obj_list( ch, arg, ch->in_room->contents )) == NULL)
    {
        send_to_char("You don't see that here.\n\r",ch);
        return;
    }

    if (IS_REMORT(ch) && obj->item_type != ITEM_CORPSE_NPC )
    {
        send_to_char("Not in remort, chucklehead.\n\r",ch);
        return;
    }

    if ( (god_name = get_god_name(ch)) == NULL )
    {
        god_name = clan_table[ch->clan].patron;
        if ( god_name == NULL || god_name[0] == '\0' )
            god_name = "Rimbol";
    }

    if ( arg[0] == '\0' || !str_cmp( arg, ch->name ) )
    {
        sprintf(buf,"$n offers $mself to %s, who graciously declines.",
                god_name);
        act( buf, ch, NULL, NULL, TO_ROOM );
        sprintf(buf,"%s appreciates your offer and may accept it later.\n\r",
                god_name);
        send_to_char(buf, ch );
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_DONATION))
    {
        send_to_char("You cannot sacrifice in a donation room.\n\r",ch);
        return;
    }

    if ( obj == NULL )
    {
        send_to_char( "You can't find it.\n\r", ch );
        return;
    }

    if (!can_loot(ch, obj, TRUE))
    {
        send_to_char("You can't sacrifice that item.\n\r",ch);
        return;
    }

    if ( obj->item_type != ITEM_CORPSE_NPC )
    {
        if (obj->contains)
        {
            sprintf(buf, "%s wouldn't like that.\n\r", god_name);
            send_to_char(buf,ch);
            return;
        }
    }

    if ( !CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC) || IS_OBJ_STAT(obj, ITEM_QUESTEQ) || is_relic_obj(obj) )
    {
        act( "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR );
        return;
    }

    if (obj->in_room != NULL)
    {
        for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
            if (gch->on == obj)
            {
                act("$N appears to be using $p.",
                        ch,obj,gch,TO_CHAR);
                return;
            }
    }

    if ( !op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_SACRIFICE) )
        return;

    silver = UMAX(1,obj->level * 3);

    if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
        silver = UMIN(silver,obj->cost);

    if (silver == 1)
    {
        sprintf(buf, "%s gives you one silver coin for your sacrifice.\n\r",
                god_name );
        send_to_char(buf, ch);
    }
    else
    {
        sprintf(buf,"%s gives you %d silver coins for your sacrifice.\n\r",
                god_name, silver);
        send_to_char(buf,ch);
    }

    ch->silver += silver;

    if ( (obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC) )
    {
        int power = dice( obj->level, obj->level ) / 10;
        int skill = get_skill( ch, gsn_drain_life );

        if ( skill > 0 && ch->hit < ch->max_hit && !PLR_ACT(ch, PLR_WAR) && obj->timer != -1 )
        {
            int hp = 2 + 2 * power * skill/100;
            ch->hit = UMIN(ch->hit + hp, ch->max_hit);
            sprintf(buf,"You drain %d hp from the corpse.\n\r", hp);
            send_to_char(buf, ch);
            change_align(ch,-2);
        }

        if ( IS_AFFECTED(ch, AFF_RITUAL) ) 
        {
            int mp = skill_table[gsn_ritual].min_mana + power;
            ch->mana = UMIN(ch->mana + mp, 11*ch->max_mana/10);
            sprintf(buf,"Your sacrifice is worth %d mana.\n\r",mp);
            send_to_char(buf, ch);
            affect_strip( ch, gsn_ritual );
            change_align(ch,-10);
        }

    }

    if (IS_SET(ch->act,PLR_AUTOSPLIT) )
    { /* AUTOSPLIT code */
        members = 0;
        for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
        {
            if ( is_same_group( gch, ch ) )
                members++;
        }

        if ( members > 1 && silver > 1)
        {
            sprintf(buffer,"%d",silver);
            do_split(ch,buffer);    
        }
    }

    sprintf(buf, "$n sacrifices $p to %s.", god_name);
    act( buf, ch, obj, NULL, TO_ROOM );
    wiznet("$N sends up $p as a burnt offering.",
            ch,obj,WIZ_SACCING,0,0);

    /* faith reward */
    worth = get_obj_faith_worth( obj );
    if ( number_range(0, 999) < worth%1000 )
        worth = worth/1000 + 1;
    else
        worth = worth/1000;
    if ( worth > 0 )
    {
        gain_faith( ch, worth );
        /* find loop-holes for faith-gaining */
        if ( worth > 1 && get_religion(ch) != NULL )
            logpf( "%s gains %d faith at %d for sacrificing obj %d",
                    ch->name, worth, ch->in_room->vnum, obj->pIndexData->vnum );
    }

    extract_obj( obj );
    return;
}



DEF_DO_FUN(do_quaff)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Quaff what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        send_to_char( "You do not have that potion.\n\r", ch );
        return;
    }

    if ( obj->item_type != ITEM_POTION )
    {
        send_to_char( "You can quaff only potions.\n\r", ch );
        return;
    }

    if (!can_use_obj(ch, obj))
    {
        send_to_char("You can't quaff that.\n\r",ch);
        return;
    }

    if (ch->level < obj->level)
    {
        send_to_char("This liquid is too powerful for you to drink.\n\r",ch);
        return;
    }

    if ( !op_percent_trigger( NULL, obj, NULL, ch, NULL, OTRIG_QUAFF) )
        return;

    WAIT_STATE( ch, PULSE_VIOLENCE );

    act( "$n quaffs $p.", ch, obj, NULL, TO_ROOM );
    act( "You quaff $p.", ch, obj, NULL ,TO_CHAR );

    obj_cast_spell( obj->value[1], obj->value[0], ch, obj, "self", FALSE );
    obj_cast_spell( obj->value[2], obj->value[0], ch, obj, "self", FALSE );
    obj_cast_spell( obj->value[3], obj->value[0], ch, obj, "self", FALSE );
    obj_cast_spell( obj->value[4], obj->value[0], ch, obj, "self", FALSE );

    extract_obj( obj );
    return;
}

DEF_DO_FUN(do_recite)
{
    char arg1[MAX_INPUT_LENGTH];
    OBJ_DATA *scroll;
    int i;

    argument = one_argument( argument, arg1 );

    if ( ( scroll = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
        send_to_char( "You do not have that scroll.\n\r", ch );
        return;
    }

    if ( scroll->item_type != ITEM_SCROLL )
    {
        send_to_char( "You can recite only scrolls.\n\r", ch );
        return;
    }

    if (get_skill(ch, gsn_scrolls) < 1 && !IS_SET(scroll->extra_flags, ITEM_REMORT))
    {
        send_to_char("You do not know how to recite scrolls.\n\r",ch);
        return;
    }

    if ( ch->level < scroll->level)
    {
        send_to_char("This scroll is too complex for you to comprehend.\n\r",ch);
        return;
    }

    if (!can_use_obj(ch, scroll))
    {
        send_to_char("You can't recite that.\n\r",ch);
        return;
    }

    if ( IS_AFFECTED(ch, AFF_HIDE) && !IS_AFFECTED(ch, AFF_SNEAK) )
    {
        affect_strip( ch, gsn_hide );
        REMOVE_BIT( ch->affect_field, AFF_HIDE );
        send_to_char( "You come out of hiding.\n\r", ch );
    }

    // we only fire the scroll if all of the spells would be valid casts
    for ( i = 1; i < 5; i++ )
    {
        if ( scroll->value[i] > 0 && !obj_cast_spell(scroll->value[i], scroll->value[0], ch, scroll, argument, TRUE) )
            return;
    }
    
    act( "$n recites $p.", ch, scroll, NULL, TO_ROOM );
    act( "You recite $p.", ch, scroll, NULL, TO_CHAR );

    if (number_percent() >= 20 + get_skill(ch,gsn_scrolls) * 4/5)
    {
        send_to_char("You mispronounce a syllable.\n\r",ch);
        check_improve(ch,gsn_scrolls,FALSE,2);
        extract_obj(scroll);
    }
    else
    {
        for ( i = 1; i < 5; i++ )
        {
            if ( scroll->value[i] > 0 )
                obj_cast_spell(scroll->value[i], scroll->value[0], ch, scroll, argument, FALSE);
        }
        extract_obj(scroll);
        check_improve(ch, gsn_scrolls, TRUE, 2);
    }

    WAIT_STATE( ch, 2*PULSE_VIOLENCE );
    return;
}


DEF_DO_FUN(do_brandish)
{
    OBJ_DATA *staff;

    if ( ( staff = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
        send_to_char( "You hold nothing in your hand.\n\r", ch );
        return;
    }

    if ( staff->item_type != ITEM_STAFF )
    {
        send_to_char( "You can brandish only with a staff.\n\r", ch );
        return;
    }

    if (get_skill(ch, gsn_staves) < 1 && !IS_SET(staff->extra_flags, ITEM_REMORT))
    {
        send_to_char("You do not know how to brandish staves.\n\r",ch);
        return;
    }

    if (!can_use_obj(ch, staff))
    {
        send_to_char("You can't brandish that.\n\r",ch);
        return;
    }

    if ( staff->value[2] <= 0 )
    {
        act( "$p has no more charges remaining.", ch, staff, NULL, TO_CHAR );
        return;
    }
    
    if ( !obj_cast_spell(staff->value[3], staff->value[0], ch, staff, argument, TRUE) )
        return;
    
    if ( IS_AFFECTED(ch, AFF_HIDE) && !IS_AFFECTED(ch, AFF_SNEAK) )
    {
        affect_strip( ch, gsn_hide );
        REMOVE_BIT( ch->affect_field, AFF_HIDE );
        send_to_char( "You come out of hiding.\n\r", ch );
    }

    if ( staff->value[2] > 0 )
    {
        act( "$n brandishes $p.", ch, staff, NULL, TO_ROOM );
        act( "You brandish $p.",  ch, staff, NULL, TO_CHAR );
        if ( ch->level < staff->level 
                ||   number_percent() >= 20 + get_skill(ch,gsn_staves) * 4/5)
        {
            act ("You fail to invoke $p.",ch,staff,NULL,TO_CHAR);
            act ("...and nothing happens.",ch,NULL,NULL,TO_ROOM);
            check_improve(ch,gsn_staves,FALSE,2);
        }
        else 
        {
            // unsuccessful cast (e.g. invalid target) does not use up charge
            if ( !obj_cast_spell(staff->value[3], staff->value[0], ch, staff, argument, FALSE) )
                return;
            check_improve(ch,gsn_staves,TRUE,2);
            --staff->value[2];
        }
    }

    if ( staff->value[2] <= 0 && staff->value[1] <= 1 )
    {
        act( "$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM );
        act( "Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR );
        extract_obj( staff );
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    return;
}

DEF_DO_FUN(do_zap)
{
    OBJ_DATA *wand;

    if ( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
        send_to_char( "You hold nothing in your hand.\n\r", ch );
        return;
    }

    if ( wand->item_type != ITEM_WAND )
    {
        send_to_char( "You can zap only with a wand.\n\r", ch );
        return;
    }

    if (get_skill(ch, gsn_wands) < 1 && !IS_SET(wand->extra_flags, ITEM_REMORT))
    {
        send_to_char("You do not know how to zap wands.\n\r",ch);
        return;
    }

    if (!can_use_obj(ch, wand))
    {
        send_to_char("You can't zap that.\n\r",ch);
        return;
    }

    if ( wand->value[2] <= 0 )
    {
        act( "$p has no more charges remaining.", ch, wand, NULL, TO_CHAR );
        return;
    }
    
    if ( !obj_cast_spell(wand->value[3], wand->value[0], ch, wand, argument, TRUE) )
        return;
    
    if ( IS_AFFECTED(ch, AFF_HIDE) && !IS_AFFECTED(ch, AFF_SNEAK) )
    {
        affect_strip( ch, gsn_hide );
        REMOVE_BIT( ch->affect_field, AFF_HIDE );
        send_to_char( "You come out of hiding.\n\r", ch );
    }

    if ( wand->value[2] > 0 )
    {
        act( "$n zaps $s $p.", ch, wand, NULL, TO_ROOM );
        act( "You zap your $p.", ch, wand, NULL, TO_CHAR );

        if (ch->level < wand->level 
                ||  number_percent() >= 20 + get_skill(ch,gsn_wands) * 4/5) 
        {
            act( "Your efforts with $p produce only smoke and sparks.",
                    ch,wand,NULL,TO_CHAR);
            act( "$n's efforts with $p produce only smoke and sparks.",
                    ch,wand,NULL,TO_ROOM);
            check_improve(ch,gsn_wands,FALSE,2);
        }
        else
        {
            // unsuccessful cast (e.g. invalid target) does not use up charge
            if ( !obj_cast_spell(wand->value[3], wand->value[0], ch, wand, argument, FALSE) )
                return;
            check_improve(ch,gsn_wands,TRUE,2);
            --wand->value[2];
        }
    }

    if ( wand->value[2] <= 0 && wand->value[1] <= 1 )
    {
        act( "$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM );
        act( "Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR );
        extract_obj( wand );
    }

    WAIT_STATE( ch, PULSE_VIOLENCE );

    return;
}

/* Steal command, gutted and reworked 7/97 by Rimbol */
DEF_DO_FUN(do_steal)
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int skill, chance;

    if (IS_NPC(ch))
        return;

    skill = get_skill(ch,gsn_steal);

    if (skill < 1)
    {    
        send_to_char("You do not know how to steal without being caught.\n\r",ch);
        return;
    }

    if ( IS_SET(ch->act, PLR_WAR) )
    {
        send_to_char( "Try again when the warfare is over!\n\r", ch );
        return;
    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_ARENA) )
    {
        send_to_char( "Not in this room!\n\r", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Steal what from whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( victim == ch )
    {
        send_to_char( "That's pointless.\n\r", ch );
        return;
    }

    if (is_safe(ch,victim))
        return;

    if ( IS_NPC(victim) 
            && victim->position == POS_FIGHTING)
    {
        send_to_char("Not while they are fighting someone else.\n\r",ch);
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_steal].beats );

    chance = (skill - get_skill(victim, gsn_alertness) / 4) * 2/3;
    chance += (get_curr_stat( ch, STAT_DEX ) - get_curr_stat( victim, STAT_INT )) / 8;
    chance += (ch->level - victim->level) / 2;

    if ( !IS_AWAKE(victim) )
        chance += 20;
    else if ( !check_see(victim,ch) )
        chance += 10;

    if (number_percent() < chance)
        /* Successful roll */
    {
        if ( !str_cmp( arg1, "coin"  )
                ||   !str_cmp( arg1, "coins" )
                ||   !str_cmp( arg1, "gold"  ) 
                ||   !str_cmp( arg1, "silver"))
        {
            int gold, silver;

            gold   = victim->gold   * number_percent() / 100;
            silver = victim->silver * number_percent() / 100;
            if ( gold <= 0 && silver <= 0 )
            {
                send_to_char( "You slip your hand in their purse, but you don't feel any coins.\n\r", ch );
                return;
            }

            ch->gold   += gold;
            ch->silver += silver;
            victim->silver -= silver;
            victim->gold   -= gold;

            if (silver <= 0)
                sprintf( buf, "Bingo!  You got %d gold coins.\n\r", gold );
            else if (gold <= 0)
                sprintf( buf, "Bingo!  You got %d silver coins.\n\r",silver);
            else
                sprintf(buf, "Bingo!  You got %d silver and %d gold coins.\n\r",
                        silver,gold);

            if( !IS_NPC(ch) && !IS_NPC(victim) )
                adjust_pkgrade( ch, victim, TRUE ); /* True means it's a theft */

            send_to_char( buf, ch );
            check_improve(ch,gsn_steal,TRUE,2);
            return;
        }

        if ( ( obj = get_obj_carry( victim, arg1, ch ) ) == NULL )
        {
            send_to_char( "You can't seem to find that.\n\r", ch );
            return;
        }

        if( !can_steal(ch, victim, obj, TRUE) )
            return;

        /*
           if ( !IS_NPC(victim)
           && ( !(IS_SET(victim->act, PLR_HARDCORE) && IS_SET(ch->act, PLR_HARDCORE))
           &&   !(IS_SET(victim->act, PLR_RP      ) && IS_SET(ch->act, PLR_RP      )) ) )
           {
           send_to_char( "You can't pry it away.\n\r", ch );
           return;
           }

           if( !can_drop_obj( victim, obj )
           ||   IS_SET(obj->extra_flags, ITEM_STICKY)
           ||   !can_loot(ch, obj, FALSE)
           ||   IS_SET(obj->extra_flags, ITEM_INVENTORY)
           ||   obj->level > ch->level )
           {
           send_to_char( "You can't pry it away.\n\r", ch );
           return;
           }

           if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
           {
           send_to_char( "You have your hands full.\n\r", ch );
           return;
           }

           if ( get_carry_weight(ch) + get_obj_weight( obj ) > can_carry_w( ch ) )
           {
           send_to_char( "You can't carry that much weight.\n\r", ch );
           return;
           }
         */

        if (!IS_NPC(victim))
        {
            sprintf(buf,"$N stole $p from %s.", victim->name);
            wiznet(buf, ch, obj, WIZ_FLAGS, 0, 0);
        }

        obj_from_char( obj );
        obj_to_char( obj, ch );
        act("Bingo! You snag $p from $N.",ch, obj, victim, TO_CHAR);

        if( !IS_NPC(ch) && !IS_NPC(victim) )
            adjust_pkgrade( ch, victim, TRUE ); /* True means it's a theft */

        check_improve(ch,gsn_steal,TRUE,2);
        return;
    }
    else
    {
        /* Failed roll */
        send_to_char( "Oops.\n\r", ch );

        make_visible( ch );

        if ( !IS_AWAKE(victim) )
            send_to_char( "Someone tried to steal from you.\n\r", victim );
        else
            act( "$n tried to steal from you.", ch, NULL, victim, TO_VICT );
        act( "$n tried to steal from $N.", ch, NULL, victim, TO_NOTVICT );

        switch(number_range(0,3))
        {
            case 0 :
                sprintf( buf, "%s is a lousy thief!", ch->name );
                break;
            case 1 :
                sprintf( buf, "%s couldn't rob %s way out of a paper bag!",
                        ch->name,(ch->sex == 2) ? "her" : "his");
                break;
            case 2 :
                sprintf( buf,"%s tried to rob me!",ch->name );
                break;
            case 3 :
                sprintf(buf,"Keep your hands out of there, %s!",ch->name);
                break;
        }

        if (!IS_AWAKE(victim)) 
            do_wake(victim,"");
        if (IS_AWAKE(victim))
            do_yell( victim, buf );

        if ( IS_NPC(victim) )
        {
            check_improve(ch,gsn_steal,FALSE,2);
            multi_hit( victim, ch, TYPE_UNDEFINED );
        }
        else if ( IS_SET(victim->in_room->room_flags, ROOM_LAW) )
        {
            sprintf(buf,"$N tried to steal from %s.", victim->name);
            wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);
            if ( !IS_SET(ch->act, PLR_THIEF) )
            {
                SET_BIT(ch->act, PLR_THIEF);
                send_to_char( "*** You are now a THIEF!! ***\n\r", ch );
                wiznet("$N is now a THIEF!", ch, NULL, WIZ_FLAGS, 0, 0);
            }
        }
        return;
    }
}



/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *keeper;
    SHOP_DATA *pShop;

    pShop = NULL;
    for ( keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room )
    {
        if ( IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL )
            break;
    }

    if ( pShop == NULL )
    {
        send_to_char( "You can't do that here.\n\r", ch );
        return NULL;
    }

    /*
     * Undesirables.
     */
    if ( ch->in_room->clan == 0 )
    {
        if ( !is_disguised(ch) )
        {
            if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_KILLER) )
            {
                do_say( keeper, "Killers are not welcome!" );
                sprintf( buf, "%s the KILLER is over here!", ch->name );
                REMOVE_BIT( keeper->penalty, PENALTY_NOSHOUT );
                do_yell( keeper, buf );
                return NULL;
            }

            if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_THIEF) )
            {
                do_say( keeper, "Thieves are not welcome!" );
                sprintf( buf, "%s the THIEF is over here!\n\r", ch->name );
                REMOVE_BIT( keeper->penalty, PENALTY_NOSHOUT );
                do_yell( keeper, buf );
                return NULL;
            }
        }
    }

    /*
     * Shop hours.
     */

    /* By popular demand, we're no longer checking shop hours - 8-10-14

    if ( time_info.hour < pShop->open_hour && !IS_IMMORTAL(ch))
    {
        do_say( keeper, "Sorry, I am closed. Come back later." );
        return NULL;
    }

    if ( time_info.hour > pShop->close_hour && !IS_IMMORTAL(ch) )
    {
        do_say( keeper, "Sorry, I am closed. Come back tomorrow." );
        return NULL;
    }

    */

    /*
     * Invisible or hidden people.
     */
    if ( !can_see( keeper, ch ) && !IS_IMMORTAL(ch) )
    {
        do_say( keeper, "I don't trade with folks I can't see." );
        return NULL;
    }

    return keeper;
}

/* insert an object at the right spot for the keeper */
void obj_to_keeper( OBJ_DATA *obj, CHAR_DATA *ch )
{
    OBJ_DATA *t_obj, *t_obj_next;

    /* see if any duplicates are found */
    for (t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj_next)
    {
        t_obj_next = t_obj->next_content;

        if (obj->pIndexData == t_obj->pIndexData 
                &&  !str_cmp(obj->short_descr,t_obj->short_descr))
        {
            /* if this is an unlimited item, destroy the new one */
            if (IS_OBJ_STAT(t_obj,ITEM_INVENTORY))
            {
                extract_obj(obj);
                return;
            }
            obj->cost = t_obj->cost; /* keep it standard */
            break;
        }
    }

    if (t_obj == NULL)
    {
        obj->next_content = ch->carrying;
        ch->carrying = obj;
    }
    else
    {
        obj->next_content = t_obj->next_content;
        t_obj->next_content = obj;
    }

    obj->carried_by      = ch;
    obj->in_room         = NULL;
    obj->in_obj          = NULL;
    ch->carry_number    += get_obj_number( obj );
    ch->carry_weight    += get_obj_weight( obj );
}

/* get an object from a shopkeeper's list */
OBJ_DATA *get_obj_keeper( CHAR_DATA *ch, CHAR_DATA *keeper, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = keeper->carrying; obj != NULL; obj = obj->next_content )
    {
        if (obj->wear_loc == WEAR_NONE
                &&  can_see_obj( keeper, obj )
                &&  can_see_obj(ch,obj)
                &&  is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;

            /* skip other objects of the same name */
            while (obj->next_content != NULL
                    && obj->pIndexData == obj->next_content->pIndexData
                    && !str_cmp(obj->short_descr,obj->next_content->short_descr))
                obj = obj->next_content;
        }
    }

    return NULL;
}

int get_cost( CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy )
{
    SHOP_DATA *pShop;
    int cost;

    if ( obj == NULL || ( pShop = keeper->pIndexData->pShop ) == NULL )
        return 0;

    if ( fBuy )
    {
        cost = obj->cost * UMAX(100, pShop->profit_buy) / 100;
    }
    else
    {
        OBJ_DATA *obj2;
        int itype;

        cost = 0;
        for ( itype = 0; itype < MAX_TRADE; itype++ )
        {
            if ( obj->item_type == pShop->buy_type[itype] )
            {
                cost = obj->cost * UMIN(100, pShop->profit_sell) / 100;
                break;
            }
        }

        if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
            for ( obj2 = keeper->carrying; obj2; obj2 = obj2->next_content )
            {
                if ( obj->pIndexData == obj2->pIndexData && !str_cmp(obj->short_descr,obj2->short_descr) )
                {
                    if (IS_OBJ_STAT(obj2,ITEM_INVENTORY))
                        cost /= 2;
                    else
                        cost = cost * 3 / 4;
                }
            }

        if ( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
        {
            if (obj->value[1] == 0)
                cost /= 4;
            else
                cost = cost * obj->value[2] / obj->value[1];
        }

    }

    return cost;
}

int haggle_cost( CHAR_DATA *ch, int cost, int base_cost )
{
    char buf[MSL];
    int skill, new_cost;

    skill = get_skill(ch, gsn_haggle) * (200 + get_skill(ch, gsn_appraise)) / 300;

    new_cost = (base_cost * skill + cost * (200-skill)) / 200;

    if ( new_cost == cost )
        return cost;

    sprintf( buf, "You haggle the price %s from %d to %d coins.\n\r",
            cost > new_cost ? "down" : "up",
            cost, new_cost );
    send_to_char( buf, ch );
    check_improve( ch, gsn_haggle, TRUE, 4);

    return new_cost;
}

DEF_DO_FUN(do_buy)
{
    char buf[MAX_STRING_LENGTH];
    int cost;

    if ( argument[0] == '\0' )
    {
        send_to_char( "Buy what?\n\r", ch );
        return;
    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_BOX_SHOP))
    {
        CHAR_DATA *banker;

        for ( banker = ch->in_room->people; banker; banker = banker->next_in_room )
        {
            if ( IS_NPC( banker ) && IS_SET(banker->pIndexData->act, ACT_BANKER) )
                break;
        }

        if ( !banker )
        {
            sprintf( buf, "The banker is currently not available.\n\r" );
            send_to_char( buf, ch );
            return;
        }

        if (str_cmp(argument, "box"))
        {
            sprintf(buf, "Sorry, %s, all I can sell you is a storage 'box'.",ch->name);
            do_say(banker,buf);
            return;
        }
        else if (ch->pcdata->storage_boxes < MAX_STORAGE_BOX)
        {
            int cost = 5000000; //50k gold 
            int qpcost= 250; 
            if ( ((ch->silver + 100 * ch->gold) < cost) ||
                    (ch->pcdata->questpoints < qpcost) )
            {
                sprintf(buf, "Sorry, %s, you can't afford a storage box.", ch->name);
                do_say(banker,buf);
                return;
            }
            deduct_cost(ch, cost);
            ch->pcdata->questpoints -= qpcost;
            ch->pcdata->storage_boxes += 1;
            sprintf(buf, "Congratulations, %s, you bought a new storage box.",
                    ch->name);
            do_say(banker, buf);
            do_say(banker, "Head west to find a room to access your box.");
            return;
        }
        else
        {
            sprintf(buf, "Sorry, %s, you can't buy any more boxes.", ch->name);
            do_say(banker, buf);
        }
        return;
    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
        char arg[MAX_INPUT_LENGTH];
        char buf[MAX_STRING_LENGTH];
        CHAR_DATA *pet;
        ROOM_INDEX_DATA *pRoomIndexNext;
        ROOM_INDEX_DATA *in_room;

        if ( IS_NPC(ch) )
            return;

        argument = one_argument(argument,arg);

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621)
            pRoomIndexNext = get_room_index(9706);
        else
            pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
        if ( pRoomIndexNext == NULL )
        {
            bug( "Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum );
            send_to_char( "Sorry, you can't buy that here.\n\r", ch );
            return;
        }

        in_room     = ch->in_room;
        ch->in_room = pRoomIndexNext;
        pet         = get_char_room( ch, arg );
        ch->in_room = in_room;

        if ( pet == NULL || !IS_SET(pet->act, ACT_PET) )
        {
            send_to_char( "Sorry, you can't buy that here.\n\r", ch );
            return;
        }

        if ( ch->pet != NULL )
        {
            send_to_char("You already own a pet.\n\r",ch);
            return;
        }

        cost = 10 * pet->level * pet->level;
        cost = haggle_cost( ch, cost, cost/2 );

        if ( (ch->silver + 100 * ch->gold) < cost )
        {
            send_to_char( "You can't afford it.\n\r", ch );
            return;
        }

        if ( ch->level < pet->level )
        {
            send_to_char(
                    "You're not powerful enough to master this pet.\n\r", ch );
            return;
        }

        deduct_cost(ch,cost);
        pet         = create_mobile( pet->pIndexData );
        SET_BIT(pet->act, ACT_PET);
        SET_BIT(pet->affect_field, AFF_CHARM);
        flag_clear( pet->penalty );
        SET_BIT( pet->penalty, PENALTY_NOTELL ); 
        SET_BIT( pet->penalty, PENALTY_NOSHOUT ); 
        SET_BIT( pet->penalty, PENALTY_NOCHANNEL ); 

        argument = one_argument( argument, arg );

        sprintf( buf, "%sA neck tag says 'I belong to %s'.\n\r",
                pet->description, ch->name );
        free_string( pet->description );
        pet->description = str_dup( buf );

        char_to_room( pet, ch->in_room );
        add_follower( pet, ch );
        pet->leader = ch;
        ch->pet = pet;
        send_to_char( "Enjoy your pet.\n\r", ch );
        act( "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM );
        return;
    }
    else
    {
        CHAR_DATA *keeper;
        OBJ_DATA *obj,*t_obj;
        char arg[MAX_INPUT_LENGTH];
        int number, count = 1;

        if ( ( keeper = find_keeper( ch ) ) == NULL )
            return;

        number = mult_argument(argument,arg);
        number = URANGE(0, number, 100);
        obj  = get_obj_keeper( ch,keeper, arg );
        cost = get_cost( keeper, obj, TRUE );

        if ( cost <= 0 || !can_see_obj( ch, obj ) )
        {
            act( "$n tells you 'I don't sell that -- try 'list''.",
                    keeper, NULL, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if (!IS_OBJ_STAT(obj,ITEM_INVENTORY))
        {
            for (t_obj = obj->next_content;
                    count < number && t_obj != NULL; 
                    t_obj = t_obj->next_content) 
            {
                if (t_obj->pIndexData == obj->pIndexData
                        &&  !str_cmp(t_obj->short_descr,obj->short_descr))
                    count++;
                else
                    break;
            }

            if (count < number)
            {
                act("$n tells you 'I don't have that many in stock.",
                        keeper,NULL,ch,TO_VICT);
                ch->reply = keeper;
                return;
            }
        }

        if (IS_EVIL(ch))
            cost = haggle_cost( ch, cost, obj->cost*4/5 );
        else
            cost = haggle_cost( ch, cost, obj->cost );

        if ( (ch->silver + ch->gold * 100) < cost * number )
        {
            if (number > 1)
                act("$n tells you 'You can't afford to buy that many.",
                        keeper,obj,ch,TO_VICT);
            else
                act( "$n tells you 'You can't afford to buy $p'.",
                        keeper, obj, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        /* Now lets you buy stuff a few levels above you */
        if ( obj->level > ch->level + 10 )
        {
            act( "$n tells you 'You can't use $p yet'.",
                    keeper, obj, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( !can_buy(ch, obj, FALSE) )
            return;

        if (ch->carry_number +  number * get_obj_number(obj) > can_carry_n(ch))
        {
            send_to_char( "You can't carry that many items.\n\r", ch );
            return;
        }

        if ( get_carry_weight(ch) + number * get_obj_weight(obj) > can_carry_w(ch))
        {
            send_to_char( "You can't carry that much weight.\n\r", ch );
            return;
        }

        if (number > 1)
        {
            sprintf(buf,"$n buys $p[%d].",number);
            act(buf,ch,obj,NULL,TO_ROOM);
            sprintf(buf,"You buy $p[%d] for %d silver.",number,cost * number);
            act(buf,ch,obj,NULL,TO_CHAR);
        }
        else
        {
            act( "$n buys $p.", ch, obj, NULL, TO_ROOM );
            sprintf(buf,"You buy $p for %d silver.",cost);
            act( buf, ch, obj, NULL, TO_CHAR );
        }
        deduct_cost(ch,cost * number);
        keeper->gold += cost * number/100;
        keeper->silver += cost * number - (cost * number/100) * 100;

        for (count = 0; count < number; count++)
        {
            if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
            {
                t_obj = create_object( obj->pIndexData, obj->level );
                check_enchant_obj( t_obj );
            }
            else
            {
                t_obj = obj;
                obj = obj->next_content;
                obj_from_char( t_obj );
            }

            if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj,ITEM_HAD_TIMER))
                t_obj->timer = 0;
            REMOVE_BIT(t_obj->extra_flags,ITEM_HAD_TIMER);
            obj_to_char( t_obj, ch );
            if (cost < t_obj->cost)
                t_obj->cost = cost;
        }
    }
}


DEF_DO_FUN(do_list)
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_SET(ch->in_room->room_flags, ROOM_BOX_SHOP))
    {
        CHAR_DATA *banker;

        for ( banker = ch->in_room->people; banker; banker = banker->next_in_room )
        {
            if ( IS_NPC( banker ) && IS_SET(banker->pIndexData->act, ACT_BANKER) )
                break;
        }

        if ( !banker )
        {
            sprintf( buf, "The banker is currently not available.\n\r" );
            send_to_char( buf, ch );
            return;
        }
        do_say(banker, "The only thing you can buy here is a storage box.");
        do_say(banker, "The cost is 50,000 gold and 250 quest points, and don't bother trying to haggle!");
        do_say(banker, "Each box can hold up to 100 items, and you can buy up to 9.");
        do_say(banker, "Go ahead and 'buy box' if you're interested!");
        return;

    }

    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
        ROOM_INDEX_DATA *pRoomIndexNext;
        CHAR_DATA *pet;
        bool found;

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621)
            pRoomIndexNext = get_room_index(9706);
        else
            pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );

        if ( pRoomIndexNext == NULL )
        {
            bug( "Do_list: bad pet shop at vnum %d.", ch->in_room->vnum );
            send_to_char( "You can't do that here.\n\r", ch );
            return;
        }

        found = FALSE;
        for ( pet = pRoomIndexNext->people; pet; pet = pet->next_in_room )
        {
            if ( IS_SET(pet->act, ACT_PET) )
            {
                if ( !found )
                {
                    found = TRUE;
                    send_to_char( "Pets for sale:\n\r", ch );
                }
                sprintf( buf, "[%2d] %8.2f - %s\n\r",
                        pet->level,
                        0.1 * pet->level * pet->level,
                        pet->short_descr );
                send_to_char( buf, ch );
            }
        }
        if ( !found )
            send_to_char( "Sorry, we're out of pets right now.\n\r", ch );
        return;
    }
    else
    {
        CHAR_DATA *keeper;
        OBJ_DATA *obj;
        int cost,count;
        bool found;
        char arg[MAX_INPUT_LENGTH];

        if ( ( keeper = find_keeper( ch ) ) == NULL )
            return;
        one_argument(argument,arg);

        found = FALSE;
        for ( obj = keeper->carrying; obj; obj = obj->next_content )
        {
            if ( obj->wear_loc == WEAR_NONE
                    &&   can_see_obj( ch, obj )
                    &&   ( cost = get_cost( keeper, obj, TRUE ) ) > 0 
                    &&   ( arg[0] == '\0'  
                        ||  is_name(arg,obj->name) ))
            {
                if ( !found )
                {
                    found = TRUE;
                    /* send_to_char( "[Lv Price Qty] Item\n\r", ch ); 
                     * Making this look better - Astark 
                     */
                    send_to_char("[ Lvl  Price    Qty ] Item\n\r", ch);
                }

                if (IS_OBJ_STAT(obj,ITEM_INVENTORY))
                    /* sprintf(buf,"[%2d %5d -- ] %s\n\r",
                     * Making this look better - Astark
                     */
                    sprintf(buf,"[ %3d %9.2f  -- ] %s\n\r",
                            obj->level,cost*0.01,obj->short_descr);
                else
                {
                    count = 1;

                    while (obj->next_content != NULL 
                            && obj->pIndexData == obj->next_content->pIndexData
                            && !str_cmp(obj->short_descr,
                                obj->next_content->short_descr))
                    {
                        obj = obj->next_content;
                        count++;
                    }
                    /* sprintf(buf,"[%2d %5d %2d ] %s\n\r",
                     */
                    sprintf(buf,"[ %3d %9.2f  %2d ] %s\n\r",
                            obj->level,cost*0.01,count,obj->short_descr);
                }
                send_to_char( buf, ch );
            }
        }

        if ( !found )
        {
            send_to_char( "You can't buy anything here.\n\r", ch );

            /* Helps immortals if they can't figure out why 
               the shop isn't working - Astark */

            if (IS_IMMORTAL(ch))
                send_to_char( "Make sure all items are assigned a value.\n\r", ch );
        }
        return;
    }
}



DEF_DO_FUN(do_sell)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Sell what?\n\r", ch );
        return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
        return;

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        act( "$n tells you 'You don't have that item'.",
                keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    if (!can_loot(ch, obj, FALSE))
    {
        send_to_char("You can't sell that.\n\r",ch);
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
        send_to_char( "You can't let go of it.\n\r", ch );
        return;
    }
    
    if ( obj->contains )
    {
        send_to_char( "You may want to empty it first.\n\r", ch );
        return;
    }

    if (!can_see_obj(keeper,obj))
    {
        act("$n doesn't see what you are offering.",keeper,NULL,ch,TO_VICT);
        return;
    }


    if ( ( cost = get_cost( keeper, obj, FALSE ) ) <= 0 )
    {
        act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
        return;
    }

    if (IS_GOOD(ch))
        cost = haggle_cost( ch, cost*5/4, obj->cost );
    else
        cost = haggle_cost( ch, cost, obj->cost );

    if ( cost > (keeper-> silver + 100 * keeper->gold) )
    {
        act("$n tells you 'I'm afraid I don't have enough wealth to buy $p.",
                keeper,obj,ch,TO_VICT);
        return;
    }

    act( "$n sells $p.", ch, obj, NULL, TO_ROOM );

    /* haggle */
    /*
       roll = number_percent();
       if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT) && roll < get_skill(ch,gsn_haggle))
       {
       printf_to_char(ch,"You haggle with %s.\n\r",PERS(keeper,ch));

       if (roll < (chance = get_skill (ch, gsn_appraise)))
       roll = (int)(((float)(2.0 * roll + chance)/(200.0 + chance)) * 100);

       cost += (obj->cost*roll) / 200;
       cost = UMIN(cost,95 * get_cost(keeper,obj,TRUE) / 100);
       cost = UMIN(cost,(keeper->silver + 100 * keeper->gold));

       check_improve(ch,gsn_haggle,TRUE,4);
       }
     */

    sprintf( buf, "You sell $p for %d silver and %d gold piece%s.",
            cost - (cost/100) * 100, cost/100, cost == 1 ? "" : "s" );
    act( buf, ch, obj, NULL, TO_CHAR );
    add_money( ch, cost/100, cost - (cost/100) * 100, keeper );
    deduct_cost(keeper,cost);
    if ( keeper->gold < 0 )
        keeper->gold = 0;
    if ( keeper->silver< 0)
        keeper->silver = 0;

    if ( obj->item_type == ITEM_TRASH || IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
    {
        extract_obj( obj );
    }
    else
    {
        obj_from_char( obj );
        if (obj->timer)
            SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
        else
            obj->timer = number_range(50,100);
        obj_to_keeper( obj, keeper );
    }

    return;
}

DEF_DO_FUN(do_value)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Value what?\n\r", ch );
        return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
        return;

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        act( "$n tells you 'You don't have that item'.",
                keeper, NULL, ch, TO_VICT );
        return;
    }

    if (!can_see_obj(keeper,obj))
    {
        act("$n doesn't see what you are offering.",keeper,NULL,ch,TO_VICT);
        return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
        send_to_char( "You can't let go of it.\n\r", ch );
        return;
    }

    if ( ( cost = get_cost( keeper, obj, FALSE ) ) <= 0 )
    {
        act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
        return;
    }

    sprintf( buf, 
            "$n tells you 'I'll give you %d silver and %d gold coins for $p'.", 
            cost - (cost/100) * 100, cost/100 );
    act( buf, keeper, obj, ch, TO_VICT );

    return;
}

DEF_DO_FUN(do_browse)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( ( keeper = find_keeper( ch ) ) == NULL )
        return;

    if ( arg[0] == '\0' )
    {
        send_to_char( "Browse what?\n\r", ch );
        return;
    }

    if ( (obj = get_obj_keeper(ch, keeper, arg)) == NULL
            || !can_see_obj( ch, obj ) )
    {
        act( "$n tells you 'I don't sell that -- try 'list''.",
                keeper, NULL, ch, TO_VICT );
        return;
    }

    do_say(keeper, "Ah, excellent choice.");
    describe_item(keeper, obj);
}

DEF_DO_FUN(do_identify)
{
    char arg[MAX_INPUT_LENGTH], buf[MSL];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( (keeper = find_keeper(ch)) == NULL )
        return;

    if ( arg[0] == '\0' )
    {
        send_to_char( "Identify what?\n\r", ch );
        return;
    }

    if ( (obj = get_obj_carry(ch, arg, ch)) == NULL )
    {
        ptc(ch, "You don't have that item.\n\r");
        return;
    }
    
    if ( !can_see_obj(keeper, obj) )
    {
        act("$n can't see $p.", keeper, obj, ch, TO_VICT);
        return;
    }

    // having an item identified costs 1% of it's worth
    int cost = UMAX(1, obj->cost / 100);
    int cost_gold = cost / 100;
    int cost_silver = cost % 100;
    
    sprintf(buf, "$n says 'It costs %d gold and %d silver to identify $p.'", cost_gold, cost_silver);
    act(buf, keeper, obj, NULL, TO_ROOM);

    if ( (ch->silver + ch->gold * 100) < cost )
        return;
    else
        do_say(keeper, "Pleasure doing business.");
    
    deduct_cost(ch, cost);
    add_money(keeper, cost / 100, cost % 100, ch);
    describe_item(keeper, obj);
}

void describe_item( CHAR_DATA *ch, OBJ_DATA* obj )
{
    AFFECT_DATA *paf;

    say_basic_obj_data( ch, obj );

    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->detect_level >= 0 )
        {
            show_affect(ch, paf, TRUE);
        }
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->detect_level >= 0 )
        {
            show_affect(ch, paf, TRUE);    
        }
    }
}

/* This donate command is derived from the publicly available snippet, 
   modified by Brian Castle to work with donation rooms.  The
   original code was not signed, and the original author is unknown. */
DEF_DO_FUN(do_donate)
{
    OBJ_DATA *obj;

    char arg[MAX_INPUT_LENGTH];
    char arg1[MIL]; 
    ROOM_INDEX_DATA *location;

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg1);

    if (IS_REMORT(ch))
    {
        send_to_char("Not in remort, chucklehead.\n\r",ch);
        return;
    }

    if (arg[0] == '\0' )
    {
        send_to_char("Donate what?\n\r",ch);
        return;
    }

    /* Second parameter to specify clan versus public - Astark 4-23-13 */
    if (!strcmp(arg1, "clan"))
        location = get_room_index(clan_table[ch->clan].donation);
    else
        location = get_room_index(ROOM_VNUM_DONATION);


    if (ch->position == POS_FIGHTING)
    {
        send_to_char(" You're {Yfighting!{x\n\r",ch);
        return;
    }

    if ((obj = get_obj_carry (ch, arg, ch)) == NULL)
    {
        send_to_char("You do not have that!\n\r",ch);
        return;
    }

    if (!can_drop_obj(ch, obj))
    {
        send_to_char("It's stuck to you.\n\r",ch);
        return;
    }

    if ( obj->item_type == ITEM_CORPSE_NPC
        || obj->item_type == ITEM_CORPSE_PC
        || obj->item_type == ITEM_EXPLOSIVE
        || obj->owner != NULL
        || is_relic_obj(obj)
        || contains_obj_recursive(obj, &is_questeq)
        || contains_obj_recursive(obj, &is_sticky_obj)
        || IS_OBJ_STAT(obj,ITEM_MELT_DROP))
    {
        send_to_char("You cannot donate that!\n\r",ch);
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_DONATION))
    {
        send_to_char("You're already here, just drop it.\n\r",ch);
        return;
    }

    /* More accurate donation message with new change */
    if (!strcmp(arg1, "clan"))
    {
        act("$n donates {Y$p{x to $s clanhall.",ch,obj,NULL,TO_ROOM);
        act("You donate {Y$p{x to your clanhall.",ch,obj,NULL,TO_CHAR);
    }
    else
    {
        act("$n donates {Y$p{x.",ch,obj,NULL,TO_ROOM);
        act("You donate {Y$p{x.",ch,obj,NULL,TO_CHAR);      
    }

    /* Commented out per January 2013 discussion on forums - Astark 4-23-13
       if (obj->timer)
       SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
       else
       obj->timer = number_range(100,200);

       obj->cost = 0;
     */

    obj_from_char(obj);
    obj_to_room(obj, location);

    return;
}

DEF_DO_FUN(do_balance)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    sprintf( buf, "You have %ld coins in the bank.\n\r", ch->pcdata->bank );
    send_to_char( buf, ch );
    return;
}

DEF_DO_FUN(do_deposit)
{
    CHAR_DATA *banker;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    long amnt;
    CHAR_DATA *changer;

    if (IS_NPC(ch))
        return;

    /* This check was nonsense --Bobble
       if (!IS_SET(ch->in_room->room_flags, ROOM_VNUM_BANK) ) 
       {
       sprintf( buf, "But you are not in a bank.\n\r" );
       send_to_char( buf, ch );
       return;
       }
     */

    banker = NULL;
    for ( banker = ch->in_room->people; banker; banker = banker->next_in_room )
    {
        if ( IS_NPC( banker ) && IS_SET(banker->pIndexData->act, ACT_BANKER) )
            break;
    }

    if ( !banker )
    {
        sprintf( buf, "The banker is currently not available.\n\r" );
        send_to_char( buf, ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        sprintf( buf, "How much gold do you wish to deposit?\n\r" );
        send_to_char( buf, ch );
        return;
    }

    for (changer = ch->in_room->people; changer; changer = changer->next_in_room )
    {
        if (IS_NPC(changer) && IS_SET(changer->pIndexData->act, ACT_IS_CHANGER))
            break;
    }

    /* deposit all, exchanges silver first, then deposits gold - Astark */
    if ( !str_cmp( arg, "all" ))
    {
        if ( !changer )
        {
            send_to_char( "The changer isn't available.\n\r", ch );
        }
        else
        {
            sprintf(buf,"%ld silver %s", ch->silver, changer->name);
            do_give(ch,buf);
        }
        amnt = ch->gold;
    }
    else
        amnt = atol( arg );

    if (amnt < 1)
        return;

    if ( amnt >= (ch->gold + 1) )
    {
        sprintf( buf, "%s, you do not have %ld gold coins.", ch->name, amnt );
        /*do_say( banker, buf );*/
        act_tell_char( banker, ch, buf );
        return;
    }

    ch->pcdata->bank += amnt;
    ch->gold -= amnt;
    sprintf( buf, "%s, your account now contains: %ld coins,", ch->name, ch->pcdata->bank );
    /*do_say( banker, buf );*/
    act_tell_char( banker, ch, buf );
    sprintf( buf, "after depositing: %ld coins.", amnt );
    /*do_say( banker, buf );*/
    act_tell_char( banker, ch, buf );
    return;
}

DEF_DO_FUN(do_withdraw)
{
    CHAR_DATA *banker;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    long amnt;

    if (IS_NPC(ch))
        return;

    /* This check was nonsense --Bobble
       if (!IS_SET(ch->in_room->room_flags, ROOM_VNUM_BANK) ) 
       {
       sprintf( buf, "But you are not in a bank.\n\r" );
       send_to_char( buf, ch );
       return;
       }
     */

    banker = NULL;
    for ( banker = ch->in_room->people; banker; banker = banker->next_in_room )
    {
        if ( IS_NPC( banker ) && IS_SET(banker->pIndexData->act, ACT_BANKER) )
            break;
    }

    if ( !banker )
    {
        sprintf( buf, "The banker is currently not available.\n\r" );
        send_to_char( buf, ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        sprintf( buf, "How much gold do you wish to withdraw?\n\r" );
        send_to_char( buf, ch );
        return;
    }

    amnt = atol( arg );

    if (amnt <1) return;

    if ( amnt >= (ch->pcdata->bank + 1) )
    {
        sprintf( buf, "%s, you do not have %ld gold coins in the bank.", ch->name, amnt );
        /*do_say( banker, buf );*/
        act_tell_char( banker, ch, buf );
        return;
    }

    ch->gold += amnt;
    ch->pcdata->bank -= amnt;
    sprintf( buf, "%s, your account now contains: %ld coins,", ch->name, ch->pcdata->bank );
    /*do_say( banker, buf );*/
    act_tell_char( banker, ch, buf );
    sprintf( buf, "after withdrawing: %ld coins.", amnt );
    /*do_say( banker, buf );*/
    act_tell_char( banker, ch, buf );
    return;
}

/* Explosives ala Rimbol.  Original idea from Wurm. */
DEF_DO_FUN(do_ignite)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int skill, chance;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Ignite what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg, ch ) ) == NULL )
    {
        send_to_char( "You do not have that item.\n\r", ch );
        return;
    }

    if ( obj->item_type != ITEM_EXPLOSIVE)
    {
        send_to_char( "You cannot set this to expode!\n\r", ch );
        return;
    }

    if ( obj->timer > 0 )
    {
        send_to_char( "That item has already been ignited!\n\r", ch);
        return;
    }

    skill = get_skill(ch, gsn_ignite);

    if (skill < 1)
    {
        send_to_char("You'd probably blow yourself to bits!\n\r",ch);
        return;
    }

    // level of bomb determines ease of use
    chance = skill * (100 + ch->level) / 100 - obj->level;
    chance = URANGE(5, chance, 95);

    if ( per_chance(chance) )
    {
        act( "$n ignites $p, and it begins sputtering and crackling ominously!", ch, obj, NULL, TO_ROOM );
        act( "You ignite $p, and it begins sputtering and crackling ominously!", ch, obj, NULL, TO_CHAR );
        obj->timer=2;
        SET_BIT(obj->extra_flags, ITEM_GLOW);

        check_improve(ch,gsn_ignite,TRUE,2);
        WAIT_STATE(ch,skill_table[gsn_ignite].beats);

        free_string(obj->owner);
        obj->owner = str_dup(ch->name);

        return;
    }

    if ( number_percent() < 33 )  /* Oops! */
    {
        act( "$n tries to ignite $p and a spark flies into the gunpowder!", ch, obj, NULL, TO_ROOM );
        act( "You try to ignite $p and a spark flies into the gunpowder!", ch, obj, NULL, TO_CHAR );
        free_string(obj->owner);
        obj->owner = str_dup(ch->name);
        explode(obj);
        extract_obj(obj);
    }
    else
    {
        act( "$n tries to ignite $p and fails.", ch, obj, NULL, TO_ROOM );
        act( "You try to ignite $p and fail.", ch, obj, NULL, TO_CHAR );
    }

    check_improve(ch,gsn_ignite,FALSE,2);
    WAIT_STATE(ch,skill_table[gsn_ignite].beats);

    return;
}

/* This recursively searches for a lit explosive inside */
/* containers. --Rimbol 07/97                           */
bool expl_in_container( OBJ_DATA *obj)
{
    OBJ_DATA *cur_obj;

    for ( cur_obj = obj->contains; cur_obj; cur_obj = cur_obj->next_content )
    {
        switch (cur_obj->item_type)
        {
            case ITEM_CONTAINER:
                if (expl_in_container(cur_obj)) return TRUE;
                break;
            case ITEM_EXPLOSIVE:
                if (cur_obj->timer > 0) return TRUE;
                break;
        }
    }
    return FALSE;
}

void check_bomb( CHAR_DATA *ch, OBJ_DATA *obj )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    if (obj->item_type != ITEM_EXPLOSIVE) return;
    if (obj->timer==0) return;

    for (vch = ch->in_room->people; vch!=NULL; vch=vch_next)
    {
        vch_next = vch->next_in_room;
        if ( IS_NPC( vch ) && HAS_TRIGGER ( vch, TRIG_DRBOMB ) && (ch!=vch))
        {
            set_pos( vch, POS_STANDING );
            mp_percent_trigger( vch, ch, obj,ACT_ARG_OBJ, NULL,0, TRIG_DRBOMB );
        }
    }

    return;
}


DEF_DO_FUN(do_second)
{

    /* Code originally written by Erwin S. Andreason */

    char arg[MIL];
    OBJ_DATA *obj;
    OBJ_DATA *wield;
    OBJ_DATA *second;
    char buf[MAX_STRING_LENGTH]; 

    argument = one_argument( argument, arg );

    if (arg[0] == '\0') /* empty */
    {
        send_to_char ("Wear which weapon in your off-hand?\n\r",ch);
        return;
    }

    obj = get_obj_carry (ch, arg, ch);

    if (obj == NULL)
    {
        send_to_char ("You have no such thing in your backpack.\n\r",ch);
        return;
    }

    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char ("That isnt a weapon, genius.\n\r",ch);
        return;
    }

    if (!can_use_obj(ch, obj)) /* Check ownership. */
    {
        send_to_char("It isn't yours.\n\r",ch);
        return;
    }

    if ( get_eq_char(ch, WEAR_HOLD) != NULL )
    {
        send_to_char ("You cannot use a secondary weapon while holding an item.\n\r", ch);
        return;
    }

    wield = get_eq_char(ch,WEAR_WIELD);

    if (wield == NULL) /* oops - != here was a bit wrong :) */
    {
        send_to_char ("You need to wield a primary weapon, before using a secondary one!\n\r",ch);
        return;
    }

    if ( IS_WEAPON_STAT( wield, WEAPON_TWO_HANDS) )
    {
        send_to_char( "Both hands are holding onto your primary weapon.\n\r", ch );
        return;
    }

    if ( IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS) )
    {
        send_to_char( "You can't wield a two-handed weapon in your off-hand!\n\r", ch );
        return;
    }

    if ( ch->level < obj->level )
    {
        sprintf( buf, "You must be level %d to use this object.\n\r", obj->level );
        send_to_char( buf, ch );
        act_gag( "$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM, GAG_EQUIP );
        return;
    }

    if ( get_obj_weight(obj) > ch_str_wield(ch) * 2/3 )
    {
        send_to_char( "This weapon is too heavy to be used as a secondary weapon by you.\n\r", ch );
        return;
    }

    if ((second = (get_eq_char(ch,WEAR_SECONDARY))) != NULL)
    { 
        if ( IS_SET(second->extra_flags, ITEM_NOREMOVE) )
        {
            act( "You can't remove $p.", ch, second, NULL, TO_CHAR );
            return;
        }

        unequip_char( ch, second );
        act_gag( "$n sheaths $p.", ch, second, NULL, TO_ROOM, GAG_EQUIP );
        act( "You sheath $p.", ch, second, NULL, TO_CHAR );
    }

    act_gag ("$n wields $p in $s off-hand.",ch,obj,NULL,TO_ROOM,GAG_EQUIP);
    act ("You wield $p in your off-hand.",ch,obj,NULL,TO_CHAR);
    equip_char ( ch, obj, WEAR_SECONDARY);

    if ( get_obj_weight(obj) > get_obj_weight(wield) )
        send_to_char ("Your secondary weapon is heavier than your primary one. Consider swapping them.\n\r", ch);

    return;
}

void merge_arrows( CHAR_DATA *ch, OBJ_DATA *obj1, OBJ_DATA *obj2 )
{
    int sum, max;

    /* just to be sure */
    if ( ch == NULL || obj1 == NULL || obj2 == NULL )
        return;

    if ( obj1->value[1] != obj2->value[1]
            || obj1->value[2] != obj2->value[2] )
    {
        send_to_char( "You can't merge different types of arrows.\n\r", ch );
        return;
    }

    act( "You merge your packs of $p.", ch, obj1, NULL, TO_CHAR );
    max = UMAX( MAX_ARROWS, obj1->pIndexData->value[0] );
    sum = obj1->value[0] + obj2->value[0];
    if ( sum <= max )
    {
        obj2->value[0] = sum;
        extract_obj( obj1 );
    }
    else
    {
        obj2->value[0] = max;
        obj1->value[0] = sum - max;
    }
}

/* Bobble: combine two objects */
DEF_DO_FUN(do_merge)
{
    OBJ_DATA *obj1, *obj2, *combine_obj;
    OBJ_INDEX_DATA *pObjIndex;
    char buf[MSL], arg1[MSL], arg2[MSL];
    int vnum;

    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char( "Combine which two objects?\n\r", ch );
        return;
    }

    obj1 = get_obj_carry(ch, arg1, ch);
    obj2 = get_obj_carry(ch, arg2, ch);

    if (obj1 == NULL || obj2 == NULL)
    {
        send_to_char( "You don't carry these things.\n\r", ch );
        return;
    }

    if (obj1 == obj2)
    {
        send_to_char( "You can't combine an object with itself!\n\r", ch );
        return;
    }

    if (obj1->pIndexData == obj2->pIndexData)
    {
        /* special case: arrows */
        if ( obj1->item_type == ITEM_ARROWS )
            merge_arrows( ch, obj1, obj2 );
        else
            send_to_char( "You can't combine two objects of the same type.\n\r", ch );
        return;
    }

    vnum = obj1->pIndexData->combine_vnum;

    if (vnum == 0 || vnum != obj2->pIndexData->combine_vnum)
    {
        send_to_char( "These objects don't fit together.\n\r", ch );
        return;
    }

    /* ok, so the objects can be combined.. */
    if ((pObjIndex = get_obj_index( vnum )) == NULL)
    {
        bug( "do_merge: object not found (%d)", vnum );
        return;
    }

    sprintf( buf, "You combine %s and %s to %s.\n\r",
            obj1->short_descr, obj2->short_descr, pObjIndex->short_descr );
    send_to_char( buf, ch );
    sprintf( buf, "$n combines %s and %s to %s.\n\r",
            obj1->short_descr, obj2->short_descr, pObjIndex->short_descr );
    act( buf, ch, NULL, NULL, TO_ROOM );

    extract_obj( obj1 );
    extract_obj( obj2 );
    combine_obj = create_object( pObjIndex, 0 );

    check_enchant_obj(combine_obj);

    if (CAN_WEAR( combine_obj, ITEM_TAKE ))
        obj_to_char( combine_obj, ch );
    else
        obj_to_room( combine_obj, ch->in_room );
}

/* Bobble: sire a vampire follower from a corpse */
DEF_DO_FUN(do_sire)
{
    OBJ_DATA *corpse;
    CHAR_DATA *mob;
    AFFECT_DATA af;
    int mlevel;

    if ( IS_NPC(ch) || ch->race != race_vampire )
    {
        send_to_char( "You're not a vampire.\n\r", ch );
        return;
    }

    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_WAR) )
    {
        send_to_char( "Go fight yourself!\n\r", ch );
        return;
    }

    if (IS_SET(ch->in_room->room_flags,ROOM_SAFE)
            || IS_SET(ch->in_room->room_flags,ROOM_LAW))
    {
        send_to_char("Not in this room.\n\r",ch);
        return;
    }

    if ( argument[0] == '\0' )
    {
        argument = "corpse";
    }

    if ( (corpse = get_obj_here(ch, argument)) == NULL )
    {
        send_to_char( "You don't see that corpse here.\n\r", ch );
        return;
    }

    if ( corpse->item_type != ITEM_CORPSE_NPC &&
            corpse->item_type != ITEM_CORPSE_PC )
    {
        send_to_char( "That's not a corpse.\n\r", ch );
        return;
    }

    if ( ch->pet != NULL )
    {
        send_to_char("You already control a pet.\n\r",ch);
        return;
    }

    /* ok, let's create the vampire mob */
    if ( (mob = create_mobile(get_mob_index(MOB_VNUM_VAMPIRE))) == NULL ) 
        return;

    WAIT_STATE( ch, PULSE_VIOLENCE );

    if (corpse->level <= ch->level)
        mlevel = (ch->level + corpse->level * 3) / 4;
    else
        mlevel = (ch->level * 3 + corpse->level) / 4;
    set_mob_level( mob, URANGE(1, mlevel, ch->level + 25) );
    char_to_room( mob, ch->in_room );

    /* wear eq from corpse */
    get_eq_corpse( mob, corpse );
    do_wear( mob, "all" );

    extract_obj ( corpse );
    change_align( ch, -10 );

    send_to_char( "You sire a vampire companion.\n\r", ch );
    act( "$n sires a vampire companion.", ch, NULL, NULL, TO_ROOM );
    add_follower( mob, ch );
    mob->leader  = ch;
    af.where     = TO_AFFECTS;
    af.type      = gsn_vampiric_bite;
    af.level     = ch->level;
    af.duration  = -1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( mob, &af );

    /* set as pet */
    SET_BIT(mob->act, ACT_PET);
    ch->pet = mob;
}


bool can_steal( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, bool verbose )
{

    /* If the victim is a player, and, either:
       - both the victim and the thief are not hardcore,
       - or both the victim and the thief are not rp,
       the theft of objects is not possible. */
    if ( !IS_NPC(victim)
            && ( !(IS_SET(victim->act, PLR_HARDCORE) && IS_SET(ch->act, PLR_HARDCORE))
                &&   !(IS_SET(victim->act, PLR_RP      ) && IS_SET(ch->act, PLR_RP      )) ) )
    {
        send_to_char( "You can't pry it away.\n\r", ch );
        return FALSE;
    }

    if( !can_drop_obj( victim, obj )
            || IS_SET(obj->extra_flags, ITEM_STICKY)
            || !can_loot(ch, obj, FALSE)
            || IS_SET(obj->extra_flags, ITEM_INVENTORY)
            || obj->level > ch->level + 5 )
    {
        if( verbose )
            send_to_char( "You can't pry it away.\n\r", ch );
        return FALSE;
    }

    if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
    {
        if( verbose )
            send_to_char( "You have your hands full.\n\r", ch ); 
        return FALSE;
    }

    if ( get_carry_weight(ch) + get_obj_weight( obj ) > can_carry_w( ch ) )
    {
        if( verbose )
            send_to_char( "You can't carry that much weight.\n\r", ch );
        return FALSE;
    }

    return TRUE;

}
