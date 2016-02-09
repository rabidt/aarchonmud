/********************************************************************* 
 * A random collection of utilities useful for OLC area builders,    *
 * that are not actually part of OLC.                                *
 *********************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "religion.h"

/* Locals */
DECLARE_DO_FUN(do_rstat     );
DECLARE_DO_FUN(do_mstat     );
DECLARE_DO_FUN(do_ostat     );
DECLARE_DO_FUN(do_rset      );
DECLARE_DO_FUN(do_mset      );
DECLARE_DO_FUN(do_oset      );
DECLARE_DO_FUN(do_sset      );
DECLARE_DO_FUN(do_cset      );
DECLARE_DO_FUN(do_mfind     );
DECLARE_DO_FUN(do_ofind     );
DECLARE_DO_FUN(do_slookup   );
const char* first_line( const char *str );



/* show a list of all used AreaVNUMS */
/* By The Mage */
/* Usage of buffers, page_to_char by Brian Castle. */
DEF_DO_FUN(do_fvlist)
{
    int i,j;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];

	if (!IS_BUILDER(ch, ch->in_room->area))
    {
        send_to_char("You are not a builder in this area.\n\r",ch);
        return;
    }

    argument = one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  fvlist obj   - List unused object vnums in current area.\n\r",ch);
        send_to_char("  fvlist mob   - List unused mob vnums in current area.\n\r",ch);
        send_to_char("  fvlist room  - List unused room vnums in current area.\n\r",ch);
        send_to_char("  fvlist mprog - List unused mprog vnums in current area.\n\r",ch);
        return;
    }


    buffer = new_buf();

    j=1;
    if (!str_cmp(arg,"obj") || !str_cmp(arg, "object"))
    {
        sprintf(buf,"{W Free {Cobject {Wvnum listing for area {C%s{x\n\r", ch->in_room->area->name);
        add_buf(buffer, buf);
        add_buf(buffer,"{Y========================================================{C\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
        {
            if (get_obj_index(i) == NULL) 
            {
                sprintf(buf,"%8d ",i);
                add_buf(buffer, buf);
                if (j == 6) 
                {
                    add_buf(buffer, "\n\r");
                    j=0;
                }
                j++;
            }
        }
        add_buf(buffer, "{x\n\r");
        page_to_char(buf_string(buffer),ch);
    }
    else if (!str_cmp(arg,"mob"))
    { 
        sprintf(buf,"{W Free {Cmob {Wvnum listing for area {C%s{x\n\r", ch->in_room->area->name);
        add_buf(buffer, buf);
        add_buf(buffer,"{Y========================================================{C\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 

        {
            if (get_mob_index(i) == NULL) 
            {
                sprintf(buf,"%8d ",i);
                add_buf(buffer, buf);
                if (j == 6) 
                {
                    add_buf(buffer, "\n\r");
                    j=0;
                }
                j++;
            }
        }
        add_buf(buffer, "{x\n\r");
        page_to_char(buf_string(buffer),ch);
    }
    else if (!str_cmp(arg,"room"))
    { 
        sprintf(buf,"{W Free {Croom {Wvnum listing for area {C%s{x\n\r", 
		ch->in_room->area->name);
        add_buf(buffer, buf);
        add_buf(buffer,"{Y========================================================{C\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
        {
            if (get_room_index(i) == NULL) 
            {
                sprintf(buf,"%8d ",i);
                add_buf(buffer, buf);
                if (j == 6) 
                {
                    add_buf(buffer, "\n\r");
                    j=0;
                }
                j++;
            }
        }
        add_buf(buffer, "{x\n\r");
        page_to_char(buf_string(buffer),ch);
    }
    else if (!str_cmp(arg,"mprog"))
    { 
        sprintf(buf,"{W Free {Cmprog {Wvnum listing for area {C%s{x\n\r", 
		ch->in_room->area->name);
        add_buf(buffer, buf);
        add_buf(buffer,"{Y========================================================{C\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
        {
            if (get_mprog_index(i) == NULL) 
            {
                sprintf(buf,"%8d ",i);
                add_buf(buffer, buf);
                if (j == 6) 
                {
                    add_buf(buffer, "\n\r");
                    j=0;
                }
                j++;
            }
        }
        add_buf(buffer, "{x\n\r");
        page_to_char(buf_string(buffer),ch);
    }
    else
        send_to_char("You may check for free object, mob, room or mprog vnums only.\n\r",ch);

    free_buf(buffer);
}


/* Show used vnums, with descriptions.  -Brian Castle 9/98.  
   Patterned after The Mage's fvlist command. */
DEF_DO_FUN(do_vlist)
{
    BUFFER *buffer;
    char buf[2*MAX_STRING_LENGTH];
    int i;
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *room = NULL;
    OBJ_INDEX_DATA *obj = NULL;
    MOB_INDEX_DATA *mob = NULL;
    PROG_CODE *mprog = NULL;
    PROG_CODE *oprog = NULL;
    PROG_CODE *aprog = NULL;
    PROG_CODE *rprog = NULL;

    if (!IS_BUILDER(ch, ch->in_room->area))
    {
        send_to_char("You are not a builder in this area.\n\r",ch);
        return;
    }

    argument = one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  vlist obj   - List used object vnums in current area.\n\r",ch);
        send_to_char("  vlist mob   - List used mob vnums in current area.\n\r",ch);
        send_to_char("  vlist room  - List used room vnums in current area.\n\r",ch);
        send_to_char("  vlist mprog - List used mprog vnums in current area.\n\r",ch);
        send_to_char("  vlist oprog - List used oprog vnums in current area.\n\r",ch);
        send_to_char("  vlist aprog - List used aprog vnums in current area.\n\r",ch);
        send_to_char("  vlist rprog - List used rprog vnums in current area.\n\r",ch);
        return;
    }

	buffer = new_buf();

    if (!str_cmp(arg,"obj") || !str_cmp(arg, "object"))
    {
        sprintf(buf, "{W Used {Cobject {Wvnum listing for area {C%s{x\n\r", ch->in_room->area->name);
		add_buf(buffer,buf);
        add_buf(buffer,"{Y========================================================{x\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
            if ((obj = get_obj_index(i)) != NULL) 
            {
                sprintf(buf,"{C%8d{x %s {x| %s{x\n\r", 
                        i, 
                        format_color_string(obj->short_descr, 30 ),
                        format_color_string(( obj->comments ), 30 ) );
                add_buf(buffer,buf);
            }

		page_to_char(buf_string(buffer),ch);
    }
    else if (!str_cmp(arg,"mob"))
    { 
        sprintf(buf,"{W Used {Cmob {Wvnum listing for area {C%s{x\n\r", ch->in_room->area->name);
		add_buf(buffer,buf);
        add_buf(buffer,"{Y========================================================{x\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
            if ((mob = get_mob_index(i)) != NULL) 
            {
                sprintf(buf,"{C%8d{x %s {x| %s{x\n\r", 
                        i, 
                        format_color_string(mob->short_descr, 30 ),
                        format_color_string(first_line( mob->comments ), 30 ) );
                add_buf(buffer,buf);
            }

		page_to_char(buf_string(buffer),ch);
    }
    else if (!str_cmp(arg,"room"))
    { 
        sprintf(buf,"{W Used {Croom {Wvnum listing for area {C%s{x\n\r", 
		ch->in_room->area->name);
		add_buf(buffer,buf);
        add_buf(buffer,"{Y========================================================{x\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
            if ((room = get_room_index(i)) != NULL) 
            {
                sprintf(buf,"{C%8d{x %s {x| %s{x\n\r", 
                        i, 
                        format_color_string(room->name, 30 ),
                        format_color_string(first_line( room->comments ), 30 ) );
                add_buf(buffer,buf);
            }

		page_to_char(buf_string(buffer),ch);
    }
    else if (!str_cmp(arg,"mprog"))
    { 
        sprintf(buf,"{W Used {Cmprog {Wvnum listing for area {C%s{x\n\r", 
		ch->in_room->area->name);
		add_buf(buffer,buf);
        add_buf(buffer,"{Y========================================================{x\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
            if ((mprog = get_mprog_index(i)) != NULL) 
            {
                sprintf(buf,"{C%8d{x %s\n\r", i, first_line(mprog->code));
                add_buf(buffer,buf);
            }

		page_to_char(buf_string(buffer),ch);
    }
    else if (!str_cmp(arg,"oprog"))
    {
        sprintf(buf,"{W Used {Coprog {Wvnum listing for area {C%s{x\n\r",
        ch->in_room->area->name);
        add_buf(buffer,buf);
        add_buf(buffer,"{Y========================================================{x\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++)
            if ((oprog = get_oprog_index(i)) != NULL)
            {
                sprintf(buf,"{C%8d{x %s\n\r", i, first_line(oprog->code));
                add_buf(buffer,buf);
            }

        page_to_char(buf_string(buffer),ch);
    }

    else if (!str_cmp(arg,"aprog"))
    {
        sprintf(buf,"{W Used {Caprog {Wvnum listing for area {C%s{x\n\r",
        ch->in_room->area->name);
        add_buf(buffer,buf);
        add_buf(buffer,"{Y========================================================{x\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++)
            if ((aprog = get_aprog_index(i)) != NULL)
            {
                sprintf(buf,"{C%8d{x %s\n\r", i, first_line(aprog->code));
                add_buf(buffer,buf);
            }

        page_to_char(buf_string(buffer),ch);
    }

    else if (!str_cmp(arg,"rprog"))
    {
        sprintf(buf,"{W Used {Crprog {Wvnum listing for area {C%s{x\n\r",
        ch->in_room->area->name);
        add_buf(buffer,buf);
        add_buf(buffer,"{Y========================================================{x\n\r");

        for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++)
            if ((rprog = get_rprog_index(i)) != NULL)
            {
                sprintf(buf,"{C%8d{x %s\n\r", i, first_line(rprog->code));
                add_buf(buffer,buf);
            }

        page_to_char(buf_string(buffer),ch);
    }

    else
        send_to_char("You may check for object, mob, room, mprog, oprog, or aprog vnums only.\n\r",ch);

    free_buf(buffer);
}

/* returns the first line of given string */
const char* first_line( const char* str )
{
    static char buf[MIL];
    int i = 0;

    if ( str == NULL )
    {
        bug( "first_line: NULL string given", 0 );
        buf[0] = '\0';
        return buf;
    }

    while ( i < MIL - 1 && str[i] != '\0' && str[i] != '\n' && str[i] != '\r' )
    {
        buf[i] = str[i];
        i++;
    }
    buf[i] = '\0';
    return buf;
}

/*   Open vlist command by Pwrdemon (jseldon@usa.net) of Stormbringer Mud  
 *   Syntax is simple:  openvlist
 *   Displays blocks of vnums not currently assigned to areas.
 */   
DEF_DO_FUN(do_openvlist)
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    int loop = 1, x, lowvnum = 0;
    bool found = FALSE;
    bool havehighvnum = FALSE;
    AREA_DATA *pArea;
    extern int top_vnum_room;  /* From db.c */

    buffer = new_buf();


    add_buf(buffer, "===== Unassigned Vnum List =====\n\r\n\r");
    for (x = 1; x <= top_vnum_room ; x++)
    {
        int count = 0;
        havehighvnum = FALSE;

        for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
        {
           if (x <= pArea->max_vnum && x >= pArea->min_vnum)
           {
               count++;                 /* count adds 1 if vnum in area    */
               havehighvnum = TRUE;     /* Might be the high vnum, trap it */
           }
        }

        if (x == top_vnum_room && count == 0)  /* Lame hack to catch last vnum  */
            havehighvnum = TRUE;

        if (count == 0 && lowvnum == 0)   /* no areas found in and no low  */
            lowvnum = x;                  /* vnum yet, assign this one     */

        if (lowvnum > 0 && havehighvnum)  /* Have low and high vnum now    */
        {
            sprintf(buf, "[%d] %d thru %d (count: %d)\n\r"
                    ,loop, lowvnum, x - 1, x - lowvnum);
            add_buf(buffer,buf);
            lowvnum = 0;                  /* Reset low vnum                */
            loop++;
            found = TRUE;
        }
    }

    if (!found)
        send_to_char("No free range of vnums was found.\n\r",ch);
    else
        page_to_char(buf_string(buffer),ch);

    free_buf(buffer);

  return;

}


/* RT to replace the 3 stat commands */

DEF_DO_FUN(do_stat)
{
   char arg[MAX_INPUT_LENGTH];
   const char *string;
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *location;
   CHAR_DATA *victim;

   string = one_argument(argument, arg);
   if ( arg[0] == '\0')
   {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  stat <name>\n\r",ch);
	send_to_char("  stat obj <name>\n\r",ch);
	send_to_char("  stat mob <name>\n\r",ch);
    send_to_char("  stat skills <mob name>\n\r",ch);
	send_to_char("  stat room <number>\n\r",ch);
	return;
   }

   if (!str_cmp(arg,"room"))
   {
	do_rstat(ch,string);
	return;
   }
  
   if (!str_cmp(arg,"obj"))
   {
	do_ostat(ch,string);
	return;
   }

   if(!str_cmp(arg,"char")  || !str_cmp(arg,"mob"))
   {
	do_mstat(ch,string);
	return;
   }
   
   if ( !str_cmp(arg, "skills") )
   {
        victim = get_char_world(ch, string);
        if ( !victim || !IS_NPC(victim) )
        {
            ptc(ch, "No such mob '%s'.\n\r", string);
        }
        ptc(ch, "Passive skills for %s:\n\r", victim->short_descr);
        show_skills_npc(victim, FALSE, ch);
        return;
   }
   
   /* do it the old way */

   victim = get_char_world(ch,argument);
   if (victim != NULL)
   {
       do_mstat(ch,argument);
       return;
   }

   obj = get_obj_world(ch,argument);
   if (obj != NULL)
   {
	 do_ostat(ch,argument);
	 return;
   }

  location = find_location(ch,argument);
  if (location != NULL)
  {
	do_rstat(ch,argument);
	return;
  }

  send_to_char("Nothing by that name found anywhere.\n\r",ch);
}

   



DEF_DO_FUN(do_rstat)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	OBJ_DATA *obj;
	CHAR_DATA *rch;
	int door;

	one_argument( argument, arg );
	location = ( arg[0] == '\0' ) ? ch->in_room : find_location( ch, arg );
	if ( location == NULL )
	{
	send_to_char( "No such location.\n\r", ch );
	return;
	}

	if (!is_room_owner(ch,location) && ch->in_room != location 
	&&  room_is_private( location ) && !IS_TRUSTED(ch,IMPLEMENTOR))
	{
	send_to_char( "That room is private right now.\n\r", ch );
	return;
	}

	sprintf( buf, "Name: '%s'\n\rArea: '%s'\n\r",
	location->name,
	location->area->name );
	send_to_char( buf, ch );

	sprintf( buf,
	"Vnum: %d  Sector: %d  Light: %d  Healing: %d  Mana: %d\n\r",
	location->vnum,
	location->sector_type,
	location->light,
	location->heal_rate,
	location->mana_rate );
	send_to_char( buf, ch );

	sprintf( buf,
        "Room flags: %s.\n\rDescription:\n\r%s",
        flag_bits_name(room_flags, location->room_flags),
	location->description );
	send_to_char( buf, ch );

	if ( location->extra_descr != NULL )
	{
	EXTRA_DESCR_DATA *ed;

	send_to_char( "Extra description keywords: '", ch );
	for ( ed = location->extra_descr; ed; ed = ed->next )
	{
		send_to_char( ed->keyword, ch );
		if ( ed->next != NULL )
		send_to_char( " ", ch );
	}
	send_to_char( "'.\n\r", ch );
	}

	send_to_char( "Characters:", ch );
	for ( rch = location->people; rch; rch = rch->next_in_room )
	{
	if (can_see(ch,rch))
		{
		send_to_char( " ", ch );
		one_argument( rch->name, buf );
		send_to_char( buf, ch );
	}
	}
	send_to_char( ".\n\r", ch);

	send_to_char( "Objects:   ", ch );
	for ( obj = location->contents; obj; obj = obj->next_content )
	{
	send_to_char( " ", ch );
	one_argument( obj->name, buf );
	send_to_char( buf, ch );
	}
	send_to_char( ".\n\r", ch );

	for ( door = 0; door <= 5; door++ )
	{
	EXIT_DATA *pexit;

	if ( ( pexit = location->exit[door] ) != NULL )
	{
		sprintf( buf,
            "Door: %d.  To: %d.  Key: %d.  Exit flags: %s.\n\rKeyword: '%s'.  Description: %s",
		door,
		(pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
			pexit->key,
            flag_bits_name(exit_flags, pexit->exit_info),
			pexit->keyword,
			pexit->description[0] != '\0'
			? pexit->description : "(none).\n\r" );
		send_to_char( buf, ch );
	}
	}

	return;
}



DEF_DO_FUN(do_ostat)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	AFFECT_DATA *paf;
	OBJ_DATA *obj;

	one_argument( argument, arg );

	if ( arg[0] == '\0' )
	{
	send_to_char( "Stat what?\n\r", ch );
	return;
	}

	if ( ( obj = get_obj_world( ch, argument ) ) == NULL )
	{
	send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
	return;
	}

	sprintf( buf, "Name(s): %s   Owner: %s\n\r",	obj->name, 
      obj->owner ? obj->owner : "(unrestricted)");
	send_to_char( buf, ch );

	sprintf( buf, "Vnum: %d  Type: %s  Resets: %d\n\r",
            obj->pIndexData->vnum,
	        item_name(obj->item_type), obj->pIndexData->reset_num );
	send_to_char( buf, ch );

	sprintf( buf, "Short description: %s\n\rLong description: %s\n\r",
	obj->short_descr, obj->description );
	send_to_char( buf, ch );

	sprintf( buf, "Wear type: %s\n\rExtra bits: %s\n\r",
    wear_bit_name(obj->wear_type), extra_bits_name( obj->extra_flags ) );

	send_to_char( buf, ch );

	sprintf( buf, "Number: %d/%d  Weight: %d/%d/%d (10th pounds)\n\r",
	1,           get_obj_number( obj ),
	obj->weight, get_obj_weight( obj ),get_true_weight(obj) );
	send_to_char( buf, ch );

	sprintf( buf, "Level: %d  Cost: %d  Timer: %d\n\r",
	obj->level, obj->cost, obj->timer );
	send_to_char( buf, ch );

	sprintf( buf,
	"In room: %d  In object: %s  Carried by: %s  Wear_loc: %d [%s]\n\r",
	obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
	obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
	obj->carried_by == NULL    ? "(none)" : 
		can_see(ch,obj->carried_by) ? obj->carried_by->name
					: "someone",
	obj->wear_loc,
    flag_bit_name(wear_loc_flags, obj->wear_loc) );
	send_to_char( buf, ch );

    sprintf( buf, "Clan: %s ClanRank: %d\n\r", 
            clan_table[obj->clan].name,
            obj->rank);
    send_to_char( buf, ch );

    printf_to_char( ch, "Material: %s\n\r", obj->material);

	
	sprintf( buf, "Values: %d %d %d %d %d\n\r",
	obj->value[0], obj->value[1], obj->value[2], obj->value[3],
	obj->value[4] );
	send_to_char( buf, ch );
	
	/* now give out vital statistics as per identify */
	
	switch ( obj->item_type )
	{
		case ITEM_SCROLL: 
		case ITEM_POTION:
		case ITEM_PILL:
		sprintf( buf, "Level %d spells of:", obj->value[0] );
		send_to_char( buf, ch );

		if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
		{
			send_to_char( " '", ch );
			send_to_char( skill_table[obj->value[1]].name, ch );
			send_to_char( "'", ch );
		}

		if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
		{
			send_to_char( " '", ch );
			send_to_char( skill_table[obj->value[2]].name, ch );
			send_to_char( "'", ch );
		}

		if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
		{
			send_to_char( " '", ch );
			send_to_char( skill_table[obj->value[3]].name, ch );
			send_to_char( "'", ch );
		}

		if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
		{
		send_to_char(" '",ch);
		send_to_char(skill_table[obj->value[4]].name,ch);
		send_to_char("'",ch);
		}

		send_to_char( ".\n\r", ch );
	break;

		case ITEM_WAND: 
		case ITEM_STAFF: 
		sprintf( buf, "Has %d(%d) charges of level %d",
			obj->value[1], obj->value[2], obj->value[0] );
		send_to_char( buf, ch );
	  
		if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
		{
			send_to_char( " '", ch );
			send_to_char( skill_table[obj->value[3]].name, ch );
			send_to_char( "'", ch );
		}

		send_to_char( ".\n\r", ch );
	break;

	case ITEM_DRINK_CON:
		sprintf(buf,"It holds %s-colored %s.\n\r",
		liq_table[obj->value[2]].liq_color,
		liq_table[obj->value[2]].liq_name);
		send_to_char(buf,ch);
		break;
		
	  
		case ITEM_WEAPON:
		send_to_char("Weapon type is ",ch);
		switch (obj->value[0])
		{
			case(WEAPON_EXOTIC): 
			send_to_char("exotic\n\r",ch);
			break;
			case(WEAPON_SWORD): 
			send_to_char("sword\n\r",ch);
			break;  
			case(WEAPON_DAGGER): 
			send_to_char("dagger\n\r",ch);
			break;
			case(WEAPON_SPEAR):
			send_to_char("spear/staff\n\r",ch);
			break;
			case(WEAPON_MACE): 
			send_to_char("mace/club\n\r",ch);   
			break;
		case(WEAPON_AXE): 
			send_to_char("axe\n\r",ch); 
			break;
			case(WEAPON_FLAIL): 
			send_to_char("flail\n\r",ch);
			break;
			case(WEAPON_WHIP): 
			send_to_char("whip\n\r",ch);
			break;
			case(WEAPON_POLEARM):
			send_to_char("polearm\n\r",ch);
			break;
			case(WEAPON_GUN): 
			send_to_char("gun\n\r",ch);
			break;
			case(WEAPON_BOW): 
			send_to_char("bow\n\r",ch);
			break;
			default: 
			send_to_char("unknown\n\r",ch);
			break;
		}
		sprintf(buf,"Damage is %dd%d (average %d)\n\r",
			obj->value[1],obj->value[2],
			(1 + obj->value[2]) * obj->value[1] / 2);
		send_to_char( buf, ch );

		sprintf(buf,"Damage noun is %s.\n\r",
		(obj->value[3] > 0 && obj->value[3] < MAX_DAMAGE_MESSAGE) ?
			attack_table[obj->value[3]].noun : "undefined");
		send_to_char(buf,ch);
		
		if (obj->value[4])  /* weapon flags */
		{
			sprintf(buf,"Weapons flags: %s\n\r",
			weapon_bits_name(obj->value[4]));
			send_to_char(buf,ch);
		}
	break;

		case ITEM_ARMOR:
		sprintf( buf, 
		"Armor class is %d.\n\r",
			obj->value[0] );
		send_to_char( buf, ch );
	break;

		case ITEM_CONTAINER:
			sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
				obj->value[0], obj->value[3], cont_bits_name(obj->value[1]));
			send_to_char(buf,ch);
			if (obj->value[4] != 100)
			{
				sprintf(buf,"Weight multiplier: %d%%\n\r",
			obj->value[4]);
				send_to_char(buf,ch);
			}
		break;
	}


	if ( obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL )
	{
	EXTRA_DESCR_DATA *ed;

	send_to_char( "Extra description keywords: '", ch );

	for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
	{
		send_to_char( ed->keyword, ch );
		if ( ed->next != NULL )
			send_to_char( " ", ch );
	}

	for ( ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next )
	{
		send_to_char( ed->keyword, ch );
		if ( ed->next != NULL )
		send_to_char( " ", ch );
	}

	send_to_char( "'\n\r", ch );
	}

	for ( paf = obj->affected; paf != NULL; paf = paf->next )
	{
	sprintf( buf, "Affects %s by %d, level %d",
		affect_loc_name( paf->location ), paf->modifier,paf->level );
	send_to_char(buf,ch);
	if ( paf->duration > -1)
		sprintf(buf,", %d hours.\n\r",paf->duration);
	else
		sprintf(buf,".\n\r");
	send_to_char( buf, ch );
	if (paf->bitvector)
	{
		switch(paf->where)
		{
		case TO_AFFECTS:
			sprintf(buf,"Adds %s affect.\n",
			affect_bit_name(paf->bitvector));
			break;
		case TO_WEAPON:
		    sprintf(buf,"Adds %s weapon flags.\n",
			    weapon_bits_name(paf->bitvector));
			break;
		case TO_OBJECT:
			sprintf(buf,"Adds %s object flag.\n",
			extra_bit_name(paf->bitvector));
			break;
		case TO_IMMUNE:
			sprintf(buf,"Adds immunity to %s.\n",
			imm_bit_name(paf->bitvector));
			break;
		case TO_RESIST:
			sprintf(buf,"Adds resistance to %s.\n\r",
			imm_bit_name(paf->bitvector));
			break;
		case TO_VULN:
			sprintf(buf,"Adds vulnerability to %s.\n\r",
			imm_bit_name(paf->bitvector));
			break;
		case TO_SPECIAL:
		    sprintf( buf, "Stores special %d.\n\r", paf->bitvector );
		    break;
		default:
			sprintf(buf,"Unknown bit %d: %d\n\r",
			paf->where,paf->bitvector);
			break;
		}
		send_to_char(buf,ch);
	}
	}

	for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
	{
	sprintf( buf, "Affects %s by %d, level %d.\n\r",
		affect_loc_name( paf->location ), paf->modifier,paf->level );
	send_to_char( buf, ch );
		if (paf->bitvector)
		{
			switch(paf->where)
			{
			case TO_AFFECTS:
			    sprintf(buf,"Adds %s affect.\n",
				    affect_bit_name(paf->bitvector));
			    break;
			case TO_OBJECT:
			    sprintf(buf,"Adds %s object flag.\n",
				    extra_bit_name(paf->bitvector));
			    break;
			case TO_IMMUNE:
			    sprintf(buf,"Adds immunity to %s.\n",
				    imm_bit_name(paf->bitvector));
			    break;
			case TO_RESIST:
			    sprintf(buf,"Adds resistance to %s.\n\r",
				    imm_bit_name(paf->bitvector));
			    break;
			case TO_VULN:
			    sprintf(buf,"Adds vulnerability to %s.\n\r",
				    imm_bit_name(paf->bitvector));
			    break;
			case TO_SPECIAL:
			    sprintf( buf, "Stores special %d.\n\r", paf->bitvector );
			    break;
			default:
			    sprintf(buf,"Unknown bit %d: %d\n\r",
				    paf->where,paf->bitvector);
			    break;
			}
			send_to_char(buf,ch);
		}
	}

	return;
}



DEF_DO_FUN(do_mstat)
{
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	AFFECT_DATA *paf;
	CHAR_DATA *victim;

	one_argument( argument, arg );

	if ( arg[0] == '\0' )
	{
	send_to_char( "Stat whom?\n\r", ch );
	return;
	}

	if ( ( victim = get_char_world( ch, argument ) ) == NULL )
	{
	send_to_char( "They aren't here.\n\r", ch );
	return;
	}

	sprintf( buf, "Name: %s\n\r",
	victim->name);
	send_to_char( buf, ch );

	sprintf( buf, 
	"Vnum: %d  Race: %s  Group: %d  Sex: %s  Room: %d\n\r",
	IS_NPC(victim) ? victim->pIndexData->vnum : 0,
	race_table[victim->race].name,
	IS_NPC(victim) ? victim->group : 0, sex_table[victim->sex].name,
	victim->in_room == NULL    ?        0 : victim->in_room->vnum
	);
	send_to_char( buf, ch );

	if (IS_NPC(victim))
	{
	sprintf(buf,"Count: %d  Killed: %d\n\r",
		victim->pIndexData->count,victim->pIndexData->killed);
	send_to_char(buf,ch);
	}

   if (!IS_NPC(victim) && victim->clan>0)
   {
      sprintf(buf, "Clan: %s  Rank: %s\n\r",
         clan_table[victim->clan].name, 
         clan_table[victim->clan].rank_list[victim->pcdata->clan_rank].name);
      send_to_char(buf,ch);
   }

   if (!IS_NPC(victim))
   {
       ptc(ch, "God: %s  Rank: %s  Faith: %d\n\r",
           has_god(victim) ? get_god_name(victim) : "None",
           get_ch_rank_name(victim), victim->pcdata->faith);
      /*
      RELIGION_DATA *rel;

      if( (rel = get_religion(victim)) != NULL )
      {
	   sprintf(buf, "Religion: %s  Rank: %s  Faith: %d  Last Prayer%s: %s",
		rel->name, get_ch_rank_name(victim), get_faith(victim),
		victim->pcdata->prayer_request ? " (pending)" : "",
		ctime(&victim->pcdata->prayed_at) );
	   send_to_char(buf,ch);
      }
     */
   }

   sprintf( buf,
      "Str: %d(%d)  Con: %d(%d)  Vit: %d(%d)  Agi: %d(%d)  Dex: %d(%d)\n\r",
      victim->perm_stat[STAT_STR],
      get_curr_stat(victim,STAT_STR),
      victim->perm_stat[STAT_CON],
      get_curr_stat(victim,STAT_CON),
      victim->perm_stat[STAT_VIT],
      get_curr_stat(victim,STAT_VIT),
      victim->perm_stat[STAT_AGI],
      get_curr_stat(victim,STAT_AGI),
      victim->perm_stat[STAT_DEX],
      get_curr_stat(victim,STAT_DEX) );
   send_to_char( buf, ch );
   
   sprintf( buf,
      "Int: %d(%d)  Wis: %d(%d)  Dis: %d(%d)  Cha: %d(%d)  Luc: %d(%d)\n\r",
      victim->perm_stat[STAT_INT],
      get_curr_stat(victim,STAT_INT),
      victim->perm_stat[STAT_WIS],
      get_curr_stat(victim,STAT_WIS),
      victim->perm_stat[STAT_DIS],
      get_curr_stat(victim,STAT_DIS),
      victim->perm_stat[STAT_CHA],
      get_curr_stat(victim,STAT_CHA),
      victim->perm_stat[STAT_LUC],
      get_curr_stat(victim,STAT_LUC) );
   send_to_char( buf, ch );

    if (!IS_NPC(victim))
	{
   sprintf( buf,
      "Str:%d Con:%d Vit:%d Agi:%d Dex:%d Int:%d Wis:%d Dis:%d Cha:%d Luc:%d\n\r",
      victim->pcdata->original_stats[STAT_STR],
      victim->pcdata->original_stats[STAT_CON],
      victim->pcdata->original_stats[STAT_VIT],
      victim->pcdata->original_stats[STAT_AGI],
      victim->pcdata->original_stats[STAT_DEX],
      victim->pcdata->original_stats[STAT_INT],
      victim->pcdata->original_stats[STAT_WIS],
      victim->pcdata->original_stats[STAT_DIS],
      victim->pcdata->original_stats[STAT_CHA],
      victim->pcdata->original_stats[STAT_LUC] );
	send_to_char(buf, ch);
	}

	sprintf( buf, "Hp: %d/%d  Mana: %d/%d  Move: %d/%d  Practices: %d  Trains: %d\n\r",
	victim->hit,         victim->max_hit,
	victim->mana,        victim->max_mana,
	victim->move,        victim->max_move,
	IS_NPC(ch) ? 0 : victim->practice,
   IS_NPC(ch) ? 0 : victim->train);
	send_to_char( buf, ch );

	if ( victim->pcdata != NULL )
	{
	    sprintf( buf, "Trains spent:  Hp: %d  Mana: %d  Move: %d\n\r",
		     victim->pcdata->trained_hit,
		     victim->pcdata->trained_mana,
		     victim->pcdata->trained_move );
	    send_to_char( buf, ch );
	}

    ptc(ch, "Lvl: %d  Class: %s  Subclass: %s  Exp: %d\n\r",
        victim->level,       
        IS_NPC(victim) ? "mobile" : class_table[victim->class].name,
        IS_NPC(victim) ? "None" : subclass_table[victim->pcdata->subclass].name);

    ptc(ch, "Align: %d  Gold: %ld  Silver: %ld\n\r",
        victim->alignment, victim->gold, victim->silver, victim->exp );

    ptc(ch, "Armor: %d  Heavy Armor: %d\n\r", GET_AC(victim), victim->heavy_armor);

	sprintf( buf, 
	"Hit: %d  Dam: %d  Saves: %d  Physical: %d  Size: %s  Position: %s\n\r",
	GET_HITROLL(victim), GET_DAMROLL(victim), get_save(victim, FALSE), get_save(victim, TRUE),
	size_table[victim->size].name, position_table[victim->position].name);
	send_to_char( buf, ch );

    sprintf( buf, "Wimpy: %d  Calm: %d\n\r", victim->wimpy, victim->calm );
    send_to_char( buf, ch );

	if (IS_NPC(victim))
	{
	sprintf(buf, "Damage: %dd%d  Message:  %s\n\r",
		victim->damage[DICE_NUMBER],victim->damage[DICE_TYPE],
		attack_table[victim->dam_type].noun);
	send_to_char(buf,ch);
	}
    sprintf( buf, "Fighting: %s  Wait: %d  Daze: %d  Stop: %d\n\r",
        victim->fighting ? victim->fighting->name : "(none)",
        victim->wait, victim->daze, victim->stop
    );
    send_to_char( buf, ch );
    
    if ( victim->stance != 0 )
        printf_to_char(ch, "Stance: %s\n\r", capitalize(stances[victim->stance].name));

	if ( !IS_NPC(victim) )
	{
	sprintf( buf,
		"Thirst: %d  Hunger: %d  Full: %d  Drunk: %d  Deep Sleep: %d  Bounty: %d\n\r",
		victim->pcdata->condition[COND_THIRST],
		victim->pcdata->condition[COND_HUNGER],
		victim->pcdata->condition[COND_FULL],
		victim->pcdata->condition[COND_DRUNK],
		victim->pcdata->condition[COND_DEEP_SLEEP],
		victim->pcdata->bounty );
	send_to_char( buf, ch );
	}

	sprintf( buf, "Carry number: %d  Carry weight: %ld\n\r",
	victim->carry_number, get_carry_weight(victim) / 10 );
	send_to_char( buf, ch );


	if (!IS_NPC(victim))
	{
	    sprintf( buf, 
		     "Age: %d  Played: %d  Last Level: %d  QPoints: %d  Timer: %d  Highest Level: %d\n\r",
		get_age(victim), 
		(int) (victim->played + current_time - victim->logon) / 3600, 
		victim->pcdata->last_level / 3600,
		victim->pcdata->questpoints,
		victim->timer,
		victim->pcdata->highest_level);
		send_to_char( buf, ch );
        printf_to_char(ch, "Authed By: %s  Demerits: %d  Spouse: %s\n\r",
            (victim->pcdata->authed_by) ? victim->pcdata->authed_by : "(unknown)", 
            victim->pcdata->demerit_points,
            victim->pcdata->spouse ? victim->pcdata->spouse : "(none)");
	}

	sprintf(buf, "Act: %s\n\r",act_bits_name(victim->act));
	send_to_char(buf,ch);
	
	if (victim->comm)
	{
		sprintf(buf,"Comm: %s\n\r",comm_bits_name(victim->comm));
		send_to_char(buf,ch);
	}

	if (victim->penalty)
	{
		sprintf(buf,"Penalty: %s\n\r",penalty_bits_name(victim->penalty));
		send_to_char(buf,ch);
	}

	if (IS_NPC(victim) && victim->off_flags)
	{
		sprintf(buf, "Offense: %s\n\r",off_bits_name(victim->off_flags));
	send_to_char(buf,ch);
	}

	if (victim->imm_flags)
	{
	sprintf(buf, "Immune: %s\n\r",imm_bits_name(victim->imm_flags));
	send_to_char(buf,ch);
	}
 
	if (victim->res_flags)
	{
	sprintf(buf, "Resist: %s\n\r", imm_bits_name(victim->res_flags));
	send_to_char(buf,ch);
	}

	if (victim->vuln_flags)
	{
	sprintf(buf, "Vulnerable: %s\n\r", imm_bits_name(victim->vuln_flags));
	send_to_char(buf,ch);
	}

	sprintf(buf, "Form: %s\n\rParts: %s\n\r", 
	form_bits_name(victim->form), part_bits_name(victim->parts));
	send_to_char(buf,ch);

	if ( !flag_is_empty(victim->affect_field) )
	{
	    sprintf( buf, "Affected by: %s\n\r", affect_bits_name( victim->affect_field ));
	    send_to_char(buf,ch);
	}

	sprintf( buf, "Master: %s  Leader: %s  Pet: %s\n\r",
	victim->master      ? victim->master->name   : "(none)",
	victim->leader      ? victim->leader->name   : "(none)",
	victim->pet         ? victim->pet->name      : "(none)");
	send_to_char( buf, ch );

     if (!IS_NPC(victim))
     {
        sprintf( buf, "Security: %d.\n\r", victim->pcdata->security ); /* OLC */
        send_to_char( buf, ch );					   /* OLC */
     }

     if (!IS_NPC(victim))
     {
        sprintf( buf, "Mobkills: %d  Mobdeaths: %d\n\r"  
                       "Pkills: %d  Pkdeaths: %d\n\r",
                       victim->pcdata->mob_kills,
                       victim->pcdata->mob_deaths,
                       victim->pcdata->pkill_count,
                       victim->pcdata->pkill_deaths);
        send_to_char( buf, ch);

        sprintf( buf, "Remorts: %d  Beheads: %d\n\r",
                        victim->pcdata->remorts,
                        victim->pcdata->behead_cnt);
        send_to_char( buf, ch);

        sprintf( buf, "Bank: %ld\n\r",
                        victim->pcdata->bank);
        send_to_char( buf, ch);

     } 
         
 
	sprintf( buf, "Short description: %s\n\rLong  description: %s\n\r",
	victim->short_descr,
	victim->long_descr[0] != '\0' ? victim->long_descr : "(none)" );
	send_to_char( buf, ch );

	if ( IS_NPC(victim) && victim->spec_fun != 0 )
	{
	sprintf(buf,"Mobile has special procedure %s.\n\r",
		spec_name_lookup(victim->spec_fun));
	send_to_char(buf,ch);
	}

	if (IS_NPC(victim) && victim->hunting)
	{
		sprintf(buf, "Hunting victim: %s\n\r", victim->hunting);
		send_to_char(buf,ch);

	}
	
	for ( paf = victim->affected; paf != NULL; paf = paf->next )
	{
	sprintf( buf,
		"Spell: '%s' modifies %s by %d for %d hours with bit %s, level %d.\n\r",
		skill_table[(int) paf->type].name,
		affect_loc_name( paf->location ),
		paf->modifier,
		paf->duration,
		to_bit_name( paf->where, paf->bitvector ),
		paf->level
		);
	send_to_char( buf, ch );
	}

	return;
}

/* ofind and mfind replaced with vnum, vnum skill also added */

DEF_DO_FUN(do_vnum)
{
	char arg[MAX_INPUT_LENGTH];
    const char *string;

	string = one_argument(argument,arg);
 
	if (arg[0] == '\0')
	{
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  vnum obj <name>\n\r",ch);
	send_to_char("  vnum mob <name>\n\r",ch);
	send_to_char("  vnum skill <skill or spell>\n\r",ch);
	return;
	}

	if (!str_cmp(arg,"obj"))
	{
	do_ofind(ch,string);
	return;
	}

	if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
	{ 
	do_mfind(ch,string);
	return;
	}

	if (!str_cmp(arg,"skill") || !str_cmp(arg,"spell"))
	{
	do_slookup(ch,string);
	return;
	}
	/* do both */
	do_mfind(ch,argument);
	do_ofind(ch,argument);
}

/* find prog with given substring */
DEF_DO_FUN(do_progfind)
{
    char buf[MAX_STRING_LENGTH];
    PROG_CODE *prog;
    int i;

    if (!IS_BUILDER(ch, ch->in_room->area))
    {
        send_to_char("You are not a builder in this area.\n\r",ch);
        return;
    }

    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax: progfind <substring>\n\r", ch );
        return;
    }

    send_to_char( "\n\rMPROG:\n\r", ch );
    for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
    {
        if ( (prog = get_mprog_index(i)) != NULL ) 
        {
            if ( strstr(prog->code, argument) )
            {
                sprintf( buf, "[%5d] %s\n\r", prog->vnum, first_line(prog->code) );
                send_to_char_new( buf, ch, TRUE );
            }
        }
    }
    
    send_to_char( "\n\rOPROG:\n\r", ch );
    for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
    {
        if ( (prog = get_oprog_index(i)) != NULL ) 
        {
            if ( strstr(prog->code, argument) )
            {
                sprintf( buf, "[%5d] %s\n\r", prog->vnum, first_line(prog->code) );
                send_to_char_new( buf, ch, TRUE );
            }
        }
    }
    
    send_to_char( "\n\rAPROG:\n\r", ch );
    for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
    {
        if ( (prog = get_aprog_index(i)) != NULL ) 
        {
            if ( strstr(prog->code, argument) )
            {
                sprintf( buf, "[%5d] %s\n\r", prog->vnum, first_line(prog->code) );
                send_to_char_new( buf, ch, TRUE );
            }
        }
    }
    
    send_to_char( "\n\rRPROG:\n\r", ch );
    for (i = ch->in_room->area->min_vnum; i <= ch->in_room->area->max_vnum; i++) 
    {
        if ( (prog = get_rprog_index(i)) != NULL ) 
        {
            if ( strstr(prog->code, argument) )
            {
                sprintf( buf, "[%5d] %s\n\r", prog->vnum, first_line(prog->code) );
                send_to_char_new( buf, ch, TRUE );
            }
        }
    }
}

/* find links into or out of an area */
DEF_DO_FUN(do_lfind)
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    ROOM_INDEX_DATA *room, *to_room;
    EXIT_DATA *exit;
    AREA_DATA *area;
    int i, door;
    bool check_in, check_out;

    area = ch->in_room->area;
    if (!IS_BUILDER(ch, area))
    {
        send_to_char("You are not a builder in this area.\n\r",ch);
        return;
    }

    if ( argument[0] == '\0' )
    {
	check_in = TRUE;
	check_out = TRUE;
    }
    else if ( !str_cmp(argument, "in") )
    {
	check_in = TRUE;
	check_out = FALSE;
    }
    else if ( !str_cmp(argument, "out") )
    {
	check_in = FALSE;
	check_out = TRUE;
    }
    else
    {
	send_to_char( "Syntax: lfind [|in|out]\n\r", ch );
	return;
    }


    buffer = new_buf();

    /* list links out of the area */
    if ( check_out )
    {
	add_buf( buffer, "Links leading out of the area:\n\r" );
	for ( i = area->min_vnum; i <= area->max_vnum; i++ )
	{
	    if ( (room=get_room_index(i)) == NULL )
		continue;
	    for ( door = 0; door < MAX_DIR; door++ )
	    {
		if ( (exit=room->exit[door]) == NULL )
		    continue;
		to_room = exit->u1.to_room;
		if ( to_room->area != area )
		{
		    sprintf( buf, "Room %d leads %s to %d in %s\n\r",
			     i, dir_name[door], to_room->vnum, to_room->area->name );
		    add_buf( buffer, buf );
		}
	    }     
	}
    }

    /* list links into the area */
    if ( check_in )
    {
	add_buf( buffer, "Links leading into the area:\n\r" );
	for ( i = 1; i <= top_vnum_room; i++ )
	{
	    /* skip rooms in area */
	    if ( i == area->min_vnum )
	    {
		i = area->max_vnum;
		continue;
	    }
	    if ( (room=get_room_index(i)) == NULL )
		continue;
	    for ( door = 0; door < MAX_DIR; door++ )
	    {
		if ( (exit=room->exit[door]) == NULL )
		    continue;
		to_room = exit->u1.to_room;
		if ( to_room->area == area )
		{
		    sprintf( buf, "Room %d in %s leads %s to %d\n\r",
			     i, room->area->name, dir_name[door], to_room->vnum );
		    add_buf( buffer, buf );
		}
	    }     
	}
    }

    page_to_char( buf_string(buffer), ch );
    free_buf( buffer );
}

DEF_DO_FUN(do_mfind)
{
	extern int top_mob_index;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	int vnum;
	int nMatch;
	bool fAll;
	bool found;

	one_argument( argument, arg );
	if ( arg[0] == '\0' )
	{
	send_to_char( "Find whom?\n\r", ch );
	return;
	}

	fAll    = FALSE; /* !str_cmp( arg, "all" ); */
	found   = FALSE;
	nMatch  = 0;

	/*
	 * Yeah, so iterating over all vnum's takes 10,000 loops.
	 * Get_mob_index is fast, and I don't feel like threading another link.
	 * Do you?
	 * -- Furey
	 */
	for ( vnum = 0; nMatch < top_mob_index; vnum++ )
	{
	if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
	{
		nMatch++;
		if ( fAll || is_name( argument, pMobIndex->player_name ) )
		{
		found = TRUE;
		sprintf( buf, "M [%5d] %s\n\r",
			pMobIndex->vnum, pMobIndex->short_descr );
		send_to_char( buf, ch );
		}
	}
	}

	if ( !found )
	send_to_char( "No mobiles by that name.\n\r", ch );

	return;
}



DEF_DO_FUN(do_ofind)
{
	extern int top_obj_index;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	int vnum;
	int nMatch;
	bool fAll;
	bool found;

	one_argument( argument, arg );
	if ( arg[0] == '\0' )
	{
	send_to_char( "Find what?\n\r", ch );
	return;
	}

	fAll    = FALSE; /* !str_cmp( arg, "all" ); */
	found   = FALSE;
	nMatch  = 0;

	/*
	 * Yeah, so iterating over all vnum's takes 10,000 loops.
	 * Get_obj_index is fast, and I don't feel like threading another link.
	 * Do you?
	 * -- Furey
	 */
	for ( vnum = 0; nMatch < top_obj_index; vnum++ )
	{
	if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
	{
		nMatch++;
		if ( fAll || is_name( argument, pObjIndex->name ) )
		{
		found = TRUE;
		sprintf( buf, "O [%5d] %s\n\r",
			pObjIndex->vnum, pObjIndex->short_descr );
		send_to_char( buf, ch );
		}
	}
	}

	if ( !found )
	send_to_char( "No objects by that name.\n\r", ch );

	return;
}


DEF_DO_FUN(do_owhere)
{
	char buf[MAX_INPUT_LENGTH];
	BUFFER *buffer;
	OBJ_DATA *obj;
	OBJ_DATA *in_obj;
	bool found;
	int number = 0, max_found;

	found = FALSE;
	number = 0;
	max_found = 200;

	if (argument[0] == '\0')
	{
	send_to_char("Find what?\n\r",ch);
	return;
	}

	buffer = new_buf();    

	for ( obj = object_list; obj != NULL; obj = obj->next )
	{
		if ( !can_see_obj( ch, obj ) || !is_name( argument, obj->name )
		||   (ch->level < obj->level))
			continue;
 
		for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
			;

		if ((in_obj->carried_by!=NULL) &&
		    (get_trust(ch)<in_obj->carried_by->invis_level))
			continue;
 
		found = TRUE;
		number++;
 
		if ( in_obj->carried_by != NULL && can_see(ch,in_obj->carried_by)
		     && in_obj->carried_by->in_room != NULL)
		    sprintf( buf, "%3d) [%5d] %s is carried by %s [Room %d]\n\r",
			     number, obj->pIndexData->vnum, obj->short_descr,
			     PERS(in_obj->carried_by, ch),
			     in_obj->carried_by->in_room->vnum );
		else if (in_obj->in_room != NULL && can_see_room(ch,in_obj->in_room))
		    sprintf( buf, "%3d) [%5d] %s is in %s [Room %d]\n\r",
			     number, obj->pIndexData->vnum, obj->short_descr,
			     in_obj->in_room->name, in_obj->in_room->vnum);
		else
		    sprintf( buf, "%3d) [%5d] %s is somewhere\n\r",
			     number, obj->pIndexData->vnum, obj->short_descr);
 
		buf[0] = UPPER(buf[0]);
		add_buf(buffer,buf);
 
		if (number >= max_found)
		    break;
	}
 
	if ( !found )
		send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
	else
		page_to_char(buf_string(buffer),ch);

	free_buf(buffer);
}


DEF_DO_FUN(do_mwhere)
{
	char buf[MAX_STRING_LENGTH];
	BUFFER *buffer;
	CHAR_DATA *victim;
	bool found;
	int count = 0;

	if ( argument[0] == '\0' )
	{
	DESCRIPTOR_DATA *d;

	/* show characters logged */

	buffer = new_buf();
	for (d = descriptor_list; d != NULL; d = d->next)
	{
		if (d->character != NULL 
                && (IS_PLAYING(d->connected))
		&&  d->character->in_room != NULL && can_see(ch,d->character)
		&&  can_see_room(ch,d->character->in_room))
		{
		victim = d->character;
		count++;
		if (d->original != NULL)
			sprintf(buf,"%3d) %s (in the body of %s) is in %s [%d]\n\r",
			count, d->original->name,victim->short_descr,
			victim->in_room->name,victim->in_room->vnum);
		else
			sprintf(buf,"%3d) %s is in %s [%d]\n\r",
			count, victim->name,victim->in_room->name,
			victim->in_room->vnum);
		add_buf(buffer,buf);
		}
	}

		page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
	return;
	}

	found = FALSE;
	buffer = new_buf();
	for ( victim = char_list; victim != NULL; victim = victim->next )
	{
	if ( victim->in_room != NULL
	&&   is_name( argument, victim->name )
	&&	 can_see(ch, victim)
	&&	 victim->in_room != NULL
	&&	 can_see_room(ch,victim->in_room))
	{
		found = TRUE;
		count++;
		sprintf( buf, "%3d) [%5d] %-28s [%5d] %s\n\r", count,
		IS_NPC(victim) ? victim->pIndexData->vnum : 0,
		IS_NPC(victim) ? victim->short_descr : victim->name,
		victim->in_room->vnum,
		victim->in_room->name );
		add_buf(buffer,buf);
	}
	}

	if ( !found )
	act( "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
	else
		page_to_char(buf_string(buffer),ch);

	free_buf(buffer);

	return;
}


/* RT set replaces sset, mset, oset, and rset */

DEF_DO_FUN(do_set)
{
    char arg[MAX_INPUT_LENGTH];
    
    argument = one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  set mob   <name> <field> <value>\n\r",ch);
        send_to_char("  set obj   <name> <field> <value>\n\r",ch);
        send_to_char("  set room  <room> <field> <value>\n\r",ch);
        send_to_char("  set skill <name> <spell or skill> <value>\n\r",ch);
        send_to_char("  set clan  <name> <field> <value>\n\r",ch);
        return;
    }
    
    if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
    {
        do_mset(ch,argument);
        return;
    }
    
    if (!str_prefix(arg,"skill") || !str_prefix(arg,"spell"))
    {
        do_sset(ch,argument);
        return;
    }
    
    if (!str_prefix(arg,"object"))
    {
        do_oset(ch,argument);
        return;
    }
    
    if (!str_prefix(arg,"room"))
    {
        do_rset(ch,argument);
        return;
    }


    if (!str_prefix(arg, "clan"))
    {
        do_cset(ch, argument);
        return;
    }

    /* echo syntax */
    do_set(ch,"");
}


DEF_DO_FUN(do_sset)
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value;
    int sn;
    bool fAll;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    
    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char( "Syntax:\n\r",ch);
        send_to_char( "  set skill <name> <spell or skill> <value>\n\r", ch);
        send_to_char( "  set skill <name> all <value>\n\r",ch);  
        send_to_char("   (use the name of the skill, not the number)\n\r",ch);
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if ( IS_NPC(victim) )
    {
        send_to_char( "Not on NPC's.\n\r", ch );
        return;
    }
    
    fAll = !str_cmp( arg2, "all" );
    sn   = 0;
    if ( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
    {
        send_to_char( "No such skill or spell.\n\r", ch );
        return;
    }
    
    /*
    * Snarf the value.
    */
    if ( !is_number( arg3 ) )
    {
        send_to_char( "Value must be numeric.\n\r", ch );
        return;
    }
    
    value = atoi( arg3 );
    if ( value < 0 || value > 100 )
    {
        send_to_char( "Value range is 0 to 100.\n\r", ch );
        return;
    }
    
    if ( fAll )
    {
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name != NULL )
                victim->pcdata->learned[sn] = value;
        }
    }
    else
    {
        victim->pcdata->learned[sn] = value;
    }
    
    return;
}

#define MSETNONE    0
#define MSETPCONLY  1
#define MSETNPCONLY 2
#define MSETANY     3

#define MSETFUN( field ) bool mset_ ## field ( CHAR_DATA *ch, CHAR_DATA *victim, const char *arg3, int value )

bool mset_stat( CHAR_DATA *ch, CHAR_DATA *victim, int stat, int value )
{
    int max_value = IS_IMMORTAL(victim) ? MAX_CURRSTAT : pc_race_table[victim->race].max_stats[stat] + class_bonus(victim->class, stat);
    if ( value < 1 || value > max_value )
    {
        ptc( ch, "%s range is 1 to %d.\n\r", stat_table[stat].name, max_value);
        return FALSE;
    }

    victim->perm_stat[stat] = value;
    victim->pcdata->original_stats[stat] = value;
    return TRUE;
}
#define MSETSTAT( statname, statval ) MSETFUN( statname ) \
{\
    return mset_stat( ch, victim, statval, value );\
}
MSETSTAT( str, STAT_STR)
MSETSTAT( con, STAT_CON)
MSETSTAT( vit, STAT_VIT)
MSETSTAT( agi, STAT_AGI)
MSETSTAT( dex, STAT_DEX)
MSETSTAT( int, STAT_INT)
MSETSTAT( wis, STAT_WIS)
MSETSTAT( dis, STAT_DIS)
MSETSTAT( cha, STAT_CHA)
MSETSTAT( luc, STAT_LUC)


MSETFUN ( class )
{
    int class;               
    class = class_lookup(arg3);
    if ( class == -1 )
    {
        char buf[MAX_STRING_LENGTH];
       
        strcpy( buf, "Possible classes are: " );
        for ( class = 0; class < MAX_CLASS; class++ )
        {
            if ( class > 0 )
                strcat( buf, " " );
            strcat( buf, class_table[class].name );
        }
        strcat( buf, ".\n\r" );
       
        send_to_char(buf,ch);
        return FALSE;
    }
   
    victim->class = class;
    return TRUE;

}


MSETFUN ( subclass )
{
    if ( !str_prefix(arg3, "None") )
    {
        victim->pcdata->subclass = 0;
        return TRUE;
    }
    
    int subclass = subclass_lookup(arg3);
    
    if ( subclass == 0 )
    {
        char buf[MAX_STRING_LENGTH];
       
        strcpy( buf, "Possible subclasses are: " );
        for ( subclass = 0; subclass_table[subclass].name != NULL; subclass++ )
        {
            if ( subclass > 0 )
                strcat( buf, " " );
            strcat( buf, subclass_table[subclass].name );
        }
        strcat(buf, ".\n\r");
       
        send_to_char(buf, ch);
        return FALSE;
    }
   
    victim->pcdata->subclass = subclass;
    return TRUE;
}


MSETFUN( race )
{
    int race;
    
    race = race_lookup(arg3);
    
    if ( race == 0)
    {
        send_to_char("That is not a valid race.\n\r",ch);
        return FALSE;
    }
    
    if (!IS_NPC(victim) && !race_table[race].pc_race)
    {
        send_to_char("That is not a valid player race.\n\r",ch);
        return FALSE;
    }
    
    victim->race = race;
    if ( victim->pcdata )
    {
        victim->pcdata->morph_race = 0;
        victim->pcdata->morph_time = 0;
        morph_update(victim);
    }
    reset_char(victim);
    
    return TRUE;
}


MSETFUN( sex )
{
    if ( value < 0 || value > 2 )
    {
        send_to_char( "Sex range is 0 to 2. 0 = Sexless, 1 = Male, 2 = Female\n\r", ch );
        return FALSE;
    }
    victim->sex = value;
    if (!IS_NPC(victim))
        victim->pcdata->true_sex = value;
    return TRUE;
}


MSETFUN( group )
{
    victim->group = value;
    return TRUE;
}


MSETFUN( align )
{
    if ( value < -1000 || value > 1000 )
    {
        send_to_char( "Alignment range is -1000 to 1000.\n\r", ch );
        return FALSE;
    }

    victim->alignment = value;
    return TRUE;
}


MSETFUN( hunt )
{
    CHAR_DATA *hunted = 0;

    if (victim->hunting)
        stop_hunting(ch);

    if ( str_cmp( arg3, "." ) )
        if ( (hunted = get_char_area(victim, arg3)) == NULL )
        {
            send_to_char("Mob couldn't locate the victim to hunt.\n\r", ch);
            return FALSE;
        }
    
    victim->hunting = strdup(hunted->name);
    return TRUE;
}


MSETFUN( gold )
{
    victim->gold = value;
    return TRUE;
}


MSETFUN( silver )
{
    victim->silver = value;
    return TRUE;
}


MSETFUN( bounty )
{
    victim->pcdata->bounty = value;
    update_bounty(victim);
    return TRUE;
}


MSETFUN( practice )
{
    if ( value < 0 || value > 5000 )
    {
        send_to_char( "Practice range is 0 to 5000 sessions.\n\r", ch );
        return FALSE;
    }
    victim->practice = value;
    return TRUE;

}


MSETFUN( train )
{
    if (value < 0 || value > 1000 )
    {
        send_to_char("Training session range is 0 to 1000 sessions.\n\r",ch);
        return FALSE;
    }
    victim->train = value;
    return TRUE;
}


MSETFUN( questpoints )
{
    printf_to_char(ch, "%s's quest points modified from %d to %d.\n\r",
        victim->name, victim->pcdata->questpoints, (victim->pcdata->questpoints + value));
    printf_to_char(victim, "%s has modified your quest points from %d to %d.\n\r",
        ch->name, victim->pcdata->questpoints, (victim->pcdata->questpoints + value));
    victim->pcdata->questpoints += value;
    return TRUE;
}


MSETFUN( thirst )
{
   if ( value < -1 || value > 100 )
   {
       send_to_char( "Thirst range is -1 to 100.\n\r", ch );
       return FALSE;
   }
   
   victim->pcdata->condition[COND_THIRST] = value;
   return TRUE;
}


MSETFUN( hunger )
{
    if ( value < -1 || value > 100 )
    {
        send_to_char( "Full range is -1 to 100.\n\r", ch );
        return FALSE;
    }
    
    victim->pcdata->condition[COND_HUNGER] = value;
    return TRUE;
}


MSETFUN( drunk )
{
    if ( value < -1 || value > 100 )
    {
        send_to_char( "Drunk range is -1 to 100.\n\r", ch );
        return FALSE;
    }
    
    victim->pcdata->condition[COND_DRUNK] = value;
    return TRUE;
}


MSETFUN( full )
{
   if ( value < -1 || value > 100 )
   {
       send_to_char( "Full range is -1 to 100.\n\r", ch );
       return FALSE;
   }
   
   victim->pcdata->condition[COND_FULL] = value;
   return TRUE;
}


MSETFUN( hp )
{
   if ( value < -10 || value > 30000 )
   {
       send_to_char( "Hp range is -10 to 30,000 hit points.\n\r", ch );
       return FALSE;
   }

   victim->max_hit = value;
   if (!IS_NPC(victim))
       victim->pcdata->perm_hit = value;
   return TRUE;
}


MSETFUN( mana )
{
    if ( value < 0 || value > 30000 )
    {
        send_to_char( "Mana range is 0 to 30,000 mana points.\n\r", ch );
        return FALSE;
    }

    victim->max_mana = value;
    if (!IS_NPC(victim))
        victim->pcdata->perm_mana = value;
    return TRUE;
}


MSETFUN( move )
{
    if ( value < 0 || value > 30000 )
    {
        send_to_char( "Move range is 0 to 30,000 move points.\n\r", ch );
        return FALSE;
    }

    victim->max_move = value;
    if (!IS_NPC(victim))
        victim->pcdata->perm_move = value;
    return TRUE;
}


MSETFUN( level )
{
    if ( value < 0 || value > 200 )
    {
        send_to_char( "Level range is 0 to 200.\n\r", ch );
        return FALSE;
    }
    victim->level = value;
    return TRUE;
}


MSETFUN( pkill )
{
    if ( IS_SET(victim->act, PLR_PERM_PKILL) )
    {
        if ( str_cmp( arg3, "off" ) )         
        {
            send_to_char("Pkill flag can only be turned 'off'.\n\r", ch);
            return FALSE;
        }

        if (get_trust(ch) < IMPLEMENTOR)
        {
            send_to_char( "Only an implementor can turn off permanent pkill.\n\r", ch);
            return FALSE;
        }

        REMOVE_BIT( victim->act, PLR_PERM_PKILL );
        REMOVE_BIT( victim->act, PLR_HARDCORE );
        send_to_char( "Pkill turned off.\n\r", ch );
        send_to_char( "You are no longer a pkiller.\n\r", victim );
        return TRUE;
    }
    else
    {
        ptc( ch, "%s not a player killer.\n\r", victim->name);
        return FALSE;
    }
}

MSETFUN( pkill_deaths )
{
    ptc(ch, "Pkill deaths adjusted from %d to %d.\n\r", victim->pcdata->pkill_deaths, victim->pcdata->pkill_deaths + value);
    victim->pcdata->pkill_deaths += value;
    return TRUE;
}


MSETFUN( killer )
{
    if ( IS_SET(victim->act, PLR_KILLER) )
    {
        if ( str_cmp( arg3, "off" ) )         
        {
            send_to_char("Killer flag can only be turned 'off'.\n\r", ch);
            return FALSE;
        }

        REMOVE_BIT( victim->act, PLR_KILLER );
        send_to_char( "Killer flag removed.\n\r", ch );
        send_to_char( "You are no longer a KILLER.\n\r", victim );
        return TRUE;
    }
    else
    {
        ptc( ch, "%s not a killer.\n\r", victim->name);
        return FALSE;
    }
}

MSETFUN( thief )
{
    if ( IS_SET(victim->act, PLR_THIEF) )
    {
        if ( str_cmp( arg3, "off" ) )         
        {
            send_to_char("Thief flag can only be turned 'off'.\n\r", ch);
            return FALSE;
        }

        REMOVE_BIT( victim->act, PLR_THIEF );
        send_to_char( "Thief flag removed.\n\r", ch );
        send_to_char( "You are no longer a THIEF.\n\r", victim );
        return TRUE;
    }
    else
    {
        ptc( ch, "%s not a player thief.\n\r", victim->name);
        return FALSE;
    }
}

MSETFUN( hardcore )
{
    if ( IS_SET(victim->act, PLR_HARDCORE) )
    {
        if ( str_cmp( arg3, "off" ) )         
        {
            send_to_char("Hardcore flag can only be turned 'off'.\n\r", ch);
            return FALSE;
        }

        if (get_trust(ch) < IMPLEMENTOR)
        {
            send_to_char( "Only an implementor can turn off hardcore pkill.\n\r", ch);
            return FALSE;
        }
        REMOVE_BIT( victim->act, PLR_HARDCORE );
        send_to_char( "Hardcore turned off.\n\r", ch );
        send_to_char( "You are no longer a hardcore pkiller.\n\r", victim );
        return TRUE;
    }
    else
    {
        ptc( ch, "%s not a hardcore player killer.\n\r", victim->name);
        return FALSE;
    }
}


MSETFUN( void )
{  
    if (IS_IMMORTAL(victim))
    {
        send_to_char("Can't void out an immortal!\n\r", ch);
        return FALSE;
    }
    else if ( get_trust( victim ) >= get_trust( ch ) )
    {
        send_to_char("Victim too powerful!\n\r", ch);
        return FALSE;
    }
    else if ( victim->timer >= 12 )
    {
        ptc( ch, "Timer for %s already at %d.\n\r", victim->name, victim->timer);
        return FALSE;
    }

    victim->timer=12;
    ptc(ch, "%s's timer was set to %d (will void on next tick).\n\r", victim->name, victim->timer);
    return TRUE; 
}

MSETFUN( security )
{
    if ( (value > ch->pcdata->security && get_trust(ch) < IMPLEMENTOR) || value < 0 || value > 9 )
    {
        if ( ch->pcdata->security != 0 )
        {
            ptc( ch, "Valid security is 0-%d.\n\r",
                ch->pcdata->security );
        }
        else
        {
            send_to_char( "Valid security is 0 only.\n\r", ch );
        }
        return FALSE;
    }
    victim->pcdata->security = value;
    return TRUE;
}

MSETFUN( law )
{
    if ( get_trust(ch) < IMPLEMENTOR )
    {
        send_to_char( "Only IMPs can assign law Aarchons.\n\r", ch );
        return FALSE;
    }
    if ( value == 0 )
    {
        REMOVE_BIT( victim->act, PLR_LAW );
        return TRUE;
    }
    else if ( value == 1 )
    {
        SET_BIT( victim->act, PLR_LAW );
        return TRUE;
    }
    else
    {
        send_to_char( "Value must be 0 to remove or 1 to set.\n\r", ch );
        return FALSE;
    }
}

MSETFUN( ptitle )
{
    char buf[MIL];
    sprintf(buf, "%s ", arg3);
    free_string( victim->pcdata->pre_title );
    victim->pcdata->pre_title = str_dup( buf );
    return TRUE;
}

MSETFUN( namecolor )
{
    return color_name( ch, arg3, victim );
}


MSETFUN( remorts )
{
    if (value < 0 || value > 10 )
    {
        send_to_char("Remort range is 0 to 10.\n\r",ch);
        return FALSE;
    }
    victim->pcdata->remorts = value;
    return TRUE;
}

MSETFUN( ascents )
{
    if ( value < 0 )
    {
        ptc(ch, "Ascents must be a non-negative integer.\n\r");
        return FALSE;
    }
    victim->pcdata->ascents = UMAX(0, value);
    return TRUE;
}

MSETFUN( god )
{
    // no god is represented by empty string
    if ( !strcmp(arg3, "none") || !strcmp(arg3, "None") )
        arg3 = "";
    // string update - with care to avoid memory leaks
    free_string(victim->pcdata->god_name);
    victim->pcdata->god_name = str_dup(arg3);
    return TRUE;
}

MSETFUN( faith )
{
    if ( value < 0 )
    {
        ptc(ch, "Faith must be a non-negative integer.\n\r");
        return FALSE;
    }
    victim->pcdata->faith = value;
    return TRUE;
}

MSETFUN( rrank )
{
    if ( value < 0 || value > RELIGION_MAX_RANK )
    {
        ptc(ch, "Religion rank range is 0 to %d.\n\r", RELIGION_MAX_RANK);
        return FALSE;
    }
    victim->pcdata->religion_rank = value;
    return TRUE;
}


struct
{
    const char *field;
    int status;
    bool (*func)(CHAR_DATA *, CHAR_DATA *, const char *, int);
} mset_table [] =
{
    {"str",       MSETANY,      mset_str},
    {"con",       MSETANY,      mset_con},
    {"vit",       MSETANY,      mset_vit},
    {"agi",       MSETANY,      mset_agi},
    {"dex",       MSETANY,      mset_dex},
    {"int",       MSETANY,      mset_int},
    {"wis",       MSETANY,      mset_wis},
    {"dis",       MSETANY,      mset_dis},
    {"cha",       MSETANY,      mset_cha},
    {"luc",       MSETANY,      mset_luc},
    {"class",     MSETPCONLY,   mset_class},
    {"subclass",  MSETPCONLY,   mset_subclass},
    {"race",      MSETPCONLY,   mset_race},
    {"sex",       MSETANY,      mset_sex},
    {"group",     MSETNPCONLY,  mset_group},
    {"align",     MSETANY,      mset_align},
    {"hunt",      MSETANY,      mset_hunt},
    {"gold",      MSETANY,      mset_gold},
    {"silver",    MSETANY,      mset_silver}, 
    {"bounty",    MSETPCONLY,   mset_bounty},
    {"practice",  MSETPCONLY,   mset_practice},
    {"train",     MSETPCONLY,   mset_train},
    {"questpoints",MSETPCONLY,   mset_questpoints},
    {"thirst",    MSETPCONLY,   mset_thirst},
    {"hunger",    MSETPCONLY,   mset_hunger},
    {"drunk",     MSETPCONLY,   mset_drunk},
    {"full",      MSETPCONLY,   mset_full},
    {"hp",        MSETANY,      mset_hp},
    {"mana",      MSETANY,      mset_mana},
    {"move",      MSETANY,      mset_move},
    {"level",     MSETNPCONLY,  mset_level},
    {"pkill",     MSETPCONLY,   mset_pkill},
    {"pk_deaths", MSETPCONLY,   mset_pkill_deaths},
    {"killer",    MSETPCONLY,   mset_killer},
    {"thief",     MSETPCONLY,   mset_thief},
    {"hardcore",  MSETPCONLY,   mset_hardcore},
    {"void",      MSETPCONLY,   mset_void},
    {"security",  MSETPCONLY,   mset_security},
    {"law",       MSETPCONLY,   mset_law},
    {"ptitle",    MSETPCONLY,   mset_ptitle},
    {"namecolor", MSETPCONLY,   mset_namecolor},
    {"remorts",   MSETPCONLY,   mset_remorts},
    {"ascents",   MSETPCONLY,   mset_ascents},
    {"god",       MSETPCONLY,   mset_god},
    {"faith",     MSETPCONLY,   mset_faith},
    {"rrank",     MSETPCONLY,   mset_rrank},
    {NULL,        MSETNONE,     NULL}
};
   

DEF_DO_FUN(do_mset)
{
    if (IS_NPC(ch))
        return;

    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value;
    
    argument = smash_tilde_cc( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );
    
    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  set char <name> <field> <value>\n\r",ch); 
        send_to_char( "  Field being one of:\n\r",          ch );
        int i; 
        int rc;
        for ( i=0,rc=0; mset_table[i].field ; i++)
        {
            if (rc>4)
            {
                send_to_char( "\n\r", ch);
                rc=0;
            }
            if (rc==0)
            {
                send_to_char("    ", ch);
            }
            
            ptc( ch, " %s", mset_table[i].field );
            rc++;
        }        
        stc("\n\r",ch);
        return;
    }
    
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    /*
    * Snarf the value (which need not be numeric).
    */
    value = is_number( arg3 ) ? atoi( arg3 ) : -1;
    
    /*
    * Set something.
    */
    int i;
    for ( i=0; mset_table[i].field ; i++ )
    {
        if (!str_prefix(arg2, mset_table[i].field) )
        {
            switch( mset_table[i].status )
            {
                case MSETANY:
                    break;
                case MSETNPCONLY:
                    if (!IS_NPC(victim))
                    {
                        ptc(ch, "Can only set %s on NPCs.\n\r", arg2);
                        return;
                    }
                    break;
                case MSETPCONLY:
                    if (IS_NPC(victim))
                    {
                        ptc(ch, "Can only set %s on PCs.\n\r", arg2);
                        return;
                    }
                    break;
                default:
                    bugf("Invalid status in do_mset for %s: %d",
                            arg2, mset_table[i].status);
                    return;
            }

            if (mset_table[i].func( ch, victim, arg3, value ) )
            {
                ptc(ch, "%s set.\n\r", arg2 );
            }
            else
            {
                ptc(ch, "%s not set.\n\r", arg2 );
            }
                
            return;
        }
    }
        
    /*
    * Generate usage message.
    */
    do_mset( ch, "" );
    return;
}

DEF_DO_FUN(do_oset)
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int value;
    
    argument = smash_tilde_cc( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );
    
    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  set obj <object> <field> <value>\n\r",ch);
        send_to_char("  Field being one of:\n\r",               ch );
        send_to_char("    value0 value1 value2 value3 value4 (v1-v4)\n\r",  ch );
        send_to_char("    extra wear level weight cost timer owner\n\r",      ch );
        return;
    }
    
    if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
        return;
    }
    
    /*
    * Snarf the value (which need not be numeric).
    */
    value = atoi( arg3 );
    
    /*
    * Set something.
    */
    if ( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
    {
        obj->value[0] = UMIN(50,value);
        return;
    }
    
    if ( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
    {
        obj->value[1] = value;
        return;
    }
    
    if ( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
    {
        obj->value[2] = value;
        return;
    }
    
    if ( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
    {
        obj->value[3] = value;
        return;
    }
    
    if ( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
    {
        obj->value[4] = value;
        return;
    }
   
    if ( !str_prefix( arg2, "extra" ) )
    {
        int flag = flag_lookup(arg3, extra_flags);
        if ( flag == NO_FLAG )
        {
            ptc(ch, "Unknown flag '%s'.\n\r", arg3);
            return;
        }
        if ( !extra_flags[flag].settable )
        {
            ptc(ch, "The %s flag cannot be set.\n\r", extra_flags[flag].name);
            return;
        }
        TOGGLE_BIT(obj->extra_flags, flag);
        return;
    }
    
    if ( !str_prefix( arg2, "wear" ) )
    {
        //obj->wear_flags = value;
        return;
    }
    
    if ( !str_prefix( arg2, "level" ) )
    {
        obj->level = value;
        return;
    }
    
    if ( !str_prefix( arg2, "weight" ) )
    {
        obj->weight = value;
        return;
    }
    
    if ( !str_prefix( arg2, "cost" ) )
    {
        obj->cost = value;
        return;
    }
    
    if ( !str_prefix( arg2, "timer" ) )
    {
        obj->timer = value;
        return;
    }
    
    if ( !str_prefix( arg2, "owner" ) )
    {
        CHAR_DATA *owner, *wch;
        
        if (!str_prefix(arg3, "clear"))
        {
            free_string(obj->owner);
            obj->owner = NULL;
            send_to_char("Owner cleared.\n\r",ch);
            return;
        }
        
        owner = NULL;
        for ( wch = char_list; wch != NULL ; wch = wch->next )
            if (!str_cmp(wch->name,arg3))
                owner = wch;
            
            if (owner == NULL || IS_NPC(owner))
            {
                send_to_char("No such player is currently online.\n\r",ch);
                return;
            }
            
            free_string(obj->owner);
            obj->owner = str_dup(owner->name);
            send_to_char("Owner set.\n\r",ch);
            return;
    }
    
    /*
    * Generate usage message.
    */
    do_oset( ch, "" );
    return;
}



DEF_DO_FUN(do_rset)
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    int value;
    
    argument = smash_tilde_cc( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );
    
    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char( "Syntax:\n\r",ch);
        send_to_char( "  set room <location> <field> <value>\n\r",ch);
        send_to_char( "  Field being one of:\n\r",          ch );
        send_to_char( "    flags sector\n\r",               ch );
        return;
    }
    
    if ( ( location = find_location( ch, arg1 ) ) == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        return;
    }
    
    if (!is_room_owner(ch,location) && ch->in_room != location 
        &&  room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That room is private right now.\n\r",ch);
        return;
    }
    
    /*
    * Snarf the value.
    */
    if ( !is_number( arg3 ) )
    {
        send_to_char( "Value must be numeric.\n\r", ch );
        return;
    }
    value = atoi( arg3 );
    
    /*
    * Set something.
    */
    if ( !str_prefix( arg2, "flags" ) )
    {
        //location->room_flags    = value;
        return;
    }
    
    if ( !str_prefix( arg2, "sector" ) )
    {
        location->sector_type   = value;
        return;
    }
    
    /*
    * Generate usage message.
    */
    do_rset( ch, "" );
    return;
}

/* sends a warning to ch if area is a clone */
void clone_warning( CHAR_DATA *ch, AREA_DATA *area )
{
    if ( ch == NULL || area == NULL )
	return;

    if ( IS_SET(area->area_flags, AREA_CLONE) )
	send_to_char( "Warning: Area is a clone. CHANGES WILL NOT SAVE!\n\r", ch );
}

/* find 'foreign' resets and mprogs not belonging to area */
DEF_DO_FUN(do_frfind)
{
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *room;
    AREA_DATA *area;
    RESET_DATA *p;
    MOB_INDEX_DATA *mob;
    PROG_LIST *mprog;
    int i, nr, min, max;

    area = ch->in_room->area;
    if (!IS_BUILDER(ch, area))
    {
        send_to_char("You are not a builder in this area.\n\r",ch);
        return;
    }

    min = ch->in_room->area->min_vnum;
    max = ch->in_room->area->max_vnum;

    send_to_char( "Foreign resets found in the following rooms:\n\r", ch );
    for ( i = min; i <= max; i++ )
    {
	if ( (room = get_room_index(i)) == NULL )
	    continue;
	
	nr = 0;
    	for (p = room->reset_first; p != NULL; p=p->next )
	{
	    nr++;
	    if ( IS_BETWEEN(min, p->arg1, max) )
		continue;

	    sprintf( buf, "Room %5d: Reset %2d: %d\n\r", i, nr, p->arg1 );
	    send_to_char( buf, ch );
   	}
    }

    send_to_char( "Foreign mprogs found on the following mobs:\n\r", ch );
    for ( i = min; i <= max; i++ )
    {
	if ( (mob = get_mob_index(i)) == NULL )
	    continue;
	
    	for ( mprog = mob->mprogs; mprog != NULL; mprog = mprog->next )
	{
	    if ( IS_BETWEEN(min, mprog->vnum, max) )
		continue;

	    sprintf( buf, "Mob %5d: %5d\n\r", i, mprog->vnum );
	    send_to_char( buf, ch );
   	}
    }
}

