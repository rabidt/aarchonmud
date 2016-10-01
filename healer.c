/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
 ***************************************************************************/
 
/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "magic.h"

/* 4 more 'heal's added in by Sraet */

DEF_DO_FUN(do_heal)
{
    CHAR_DATA *mob;
    char arg[MAX_INPUT_LENGTH];
    int cost,sn;
    SPELL_FUN *spell;
    char *words;	
    
    /* check for healer */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
        if ( IS_NPC(mob) && IS_SET(mob->act, ACT_IS_HEALER) )
            break;
    }
    
    if ( mob == NULL )
    {
        send_to_char( "You can't do that here.\n\r", ch );
        return;
    }
    
    argument=one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        /* display price list */
        act("$N says 'I offer the following spells:'",ch,NULL,mob,TO_CHAR);
        send_to_char("  light:   cure light wounds     10 gold\n\r",ch);
        send_to_char("  serious: cure serious wounds   15 gold\n\r",ch);
        send_to_char("  critic:  cure critical wounds  25 gold\n\r",ch);
        send_to_char("  heal:    healing spell         50 gold\n\r",ch);
        send_to_char("  blind:   cure blindness        20 gold\n\r",ch);
        send_to_char("  disease: cure disease          15 gold\n\r",ch);
        send_to_char("  poison:  cure poison           10 gold\n\r",ch); 
        send_to_char("  mental:  cure mental           25 gold\n\r",ch); 
        send_to_char("  uncurse: remove curse          50 gold\n\r",ch);
        send_to_char("  cancel:  cancel spells        150 gold\n\r",ch);
        send_to_char("  refresh: restore movement       5 gold\n\r",ch);
        send_to_char("  mana:    restore mana          10 gold\n\r",ch);
        send_to_char("  body:    health to full       variable\n\r",ch);
        send_to_char("  mind:    mana to full         variable\n\r",ch);
        send_to_char("  stamina: movement to full     variable\n\r",ch);
        send_to_char("  all:     all to full          variable\n\r",ch);
        send_to_char(" Type heal <type> to be healed.\n\r",ch);
        return;
    }
    
    if (!str_prefix(arg,"cancel"))
    {
        spell = spell_cancellation;
        sn    = skill_lookup("cancellation");
        words = "azzumflat oxcart";
        cost  = 15000;
    }
    
    else if (!str_prefix(arg,"light"))
    {
        spell = spell_cure_light;
        sn    = skill_lookup("cure light");
        words = "judicandus dies";
        cost  = 1000;
    }
    
    else if (!str_prefix(arg,"serious"))
    {
        spell = spell_cure_serious;
        sn    = skill_lookup("cure serious");
        words = "judicandus gzfuajg";
        cost  = 1600;
    }
    
    else if (!str_prefix(arg,"critical"))
    {
        spell = spell_cure_critical;
        sn    = skill_lookup("cure critical");
        words = "judicandus qfuhuqar";
        cost  = 2500;
    }
    
    else if (!str_prefix(arg,"heal"))
    {
        spell = spell_heal;
        sn = skill_lookup("heal");
        words = "pzar";
        cost  = 5000;
    }
    
    else if (!str_prefix(arg,"blindness"))
    {
        spell = spell_cure_blindness;
        sn    = skill_lookup("cure blindness");
        words = "judicandus noselacri";		
        cost  = 2000;
    }
    
    else if (!str_prefix(arg,"disease"))
    {
        spell = spell_cure_disease;
        sn    = skill_lookup("cure disease");
        words = "judicandus eugzagz";
        cost = 1500;
    }
    
    else if (!str_prefix(arg,"poison"))
    {
        spell = spell_cure_poison;
        sn    = skill_lookup("cure poison");
        words = "judicandus sausabru";
        cost  = 1000;
    }
    
    else if (!str_prefix(arg,"mental"))
    {
        spell = spell_cure_mental;
        sn    = skill_lookup("cure mental");
        words = "judicandus minetus";
        cost  = 2500;
    }

    else if (!str_prefix(arg,"uncurse") || !str_prefix(arg,"curse"))
    {
        spell = spell_remove_curse; 
        sn    = skill_lookup("remove curse");
        words = "candussido judifgz";
        cost  = 5000;
    }
    
    else if (!str_prefix(arg,"mana") || !str_prefix(arg,"energize"))
    {
        spell = NULL;
        sn = -1;
        words = "energizer";
        cost = 1000;
    }
    
    
    else if (!str_prefix(arg,"refresh") || !str_prefix(arg,"moves"))
    {
        spell =  spell_refresh;
        sn    = skill_lookup("refresh");
        words = "candusima"; 
        cost  = 500;
    }
    else if (!str_prefix(arg, "body")) {
        spell = NULL;
        sn = -2;
        words = "streaaerts";
        cost = ((hit_cap(ch) - ch->hit) * 30);
    }
    else if (!str_prefix(arg, "mind")) {
        spell = NULL;
        sn = -3;
        words = "beau crysania";
        cost = ((mana_cap(ch) - ch->mana) * 30);
    }
    else if (!str_prefix(arg, "stamina")) {
        spell = NULL;
        sn = -4;
        words = "covet love";
        cost = ((move_cap(ch) - ch->move) * 30);
    }
    else if (!str_prefix(arg, "all")) {
        spell = NULL;
        sn = -5;
        words = "flin dalnib";
        cost = (hit_cap(ch) - ch->hit) * 32;
        cost += (mana_cap(ch) - ch->mana) * 32;
        cost += (move_cap(ch) - ch->move) * 32;
    }
    else 
    {
        act("$N says 'Type 'heal' for a list of spells.'",
            ch,NULL,mob,TO_CHAR);
        return;
    }
    
    // players pay for their pets
    CHAR_DATA *sponsor = get_local_leader(ch);
    
    if ( !has_money(sponsor, cost, TRUE) )
    {
        act("$N says 'You do not have enough gold for my services.'", sponsor, NULL, mob, TO_CHAR);
        return;
    }
    
    WAIT_STATE(ch,PULSE_VIOLENCE);
    
    cost = UMAX(0, cost);
    deduct_cost(sponsor, cost);
    mob->gold += cost / 100;
    mob->silver += cost % 100;

    act("$n utters the words '$T'.",mob,NULL,words,TO_ROOM);
    
    if (spell == NULL && sn == -1)  /* restore mana trap...kinda hackish */
    {
        gain_mana(ch, 100);
        send_to_char("A warm glow passes through you.\n\r",ch);
        return;
    }
    switch(sn) {
    case -2: /* Full HP */
        ch->hit = hit_cap(ch);
        send_to_char("Your blood warms as life renewed flows through you.\n\r", ch);
        return;
    case -3: /* Full Mana */
        ch->mana = mana_cap(ch);
        send_to_char("Your mind tingles as your magical strength is renewed.\n\r", ch);
        return;
    case -4: /* Full Mv */
        ch->move = move_cap(ch);
        send_to_char("You feel your stamina return to you.\n\r", ch);
        return;
    case -5: /* Full All */
        ch->hit = hit_cap(ch);
        ch->mana = mana_cap(ch);
        ch->move = move_cap(ch);
        send_to_char("Your body burns with energy as you are renewed.\n\r", ch);
        return;
    case -1: break;
    default:
        break;
    }
   
    static char tname_buf[MIL];
    sprintf( tname_buf, "%s %s",
          ch->name,
          argument);
    target_name=tname_buf;
            
    spell(sn, 120, mob, ch, TARGET_CHAR, FALSE);
}

typedef struct spell_cost SPELL_COST;
struct spell_cost
{
    char *name;
    int cost;
};

static SPELL_COST arcane_cost[] =
{
    { "sanctuary", 200 },
    { "damned blade", 150 },
    { "haste", 100 },
    { "fly", 50 },
    { "protection magic", 50 },
    { "giant strength", 20 },
    { "shield", 20 },
    { "armor", 10 },
    /*
    { "detect hidden", 10 },
    { "detect invis", 10 },
    */
    { "invisibility", 10 },
    { "breathe water", 10 },
    { NULL, 0 }
};


/* added by Bobble */
DEF_DO_FUN(do_spellup)
{
    CHAR_DATA *mob;
    char arg[MAX_INPUT_LENGTH], buf[MSL];
    int cost,sn,spell;
    
    /* check for healer */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
        if ( IS_NPC(mob) && IS_SET(mob->act, ACT_SPELLUP) )
            break;
    }
    
    if ( mob == NULL )
    {
        send_to_char( "You can't do that here.\n\r", ch );
        return;
    }
    
    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
        /* display price list */
        act("$N says 'I offer the following spells:'",ch,NULL,mob,TO_CHAR);
	for ( spell = 0; arcane_cost[spell].name != NULL; spell++ )
	{
	    sprintf( buf, "  %-18s: %5d gold\n\r",
		     arcane_cost[spell].name, arcane_cost[spell].cost );
	    send_to_char( buf, ch );
	}
        send_to_char( "  all               :  1000 gold\n\r", ch );
        send_to_char( " Type spellup <type> to be spelled up.\n\r", ch );
        return;
    }
    
    if (!str_prefix(arg, "all"))
    {
	sn = -1;
	cost = 1000;
    }
    else
    {
	for ( spell = 0; arcane_cost[spell].name != NULL; spell++ ) 
	    if ( !str_prefix(arg, arcane_cost[spell].name) )
		break;

	if ( arcane_cost[spell].name == NULL )
	{
	    act("$N says 'Type 'spellup' for a list of spells.'",
		ch,NULL,mob,TO_CHAR);
	    return;
	}

	sn = skill_lookup( arcane_cost[spell].name );
	cost = arcane_cost[spell].cost;
    }
    
    // players pay for their pets
    CHAR_DATA *sponsor = get_local_leader(ch);
    
    if ( !has_money(sponsor, cost*100, TRUE) )
    {
        act("$N says 'You do not have enough gold for my services.'", sponsor, NULL, mob, TO_CHAR);
        return;
    }
    
    WAIT_STATE(ch, PULSE_VIOLENCE);
    
    cost = UMAX(0, cost);
    deduct_cost(sponsor, cost*100);
    mob->gold += cost;

    act("$n utters some arcane words.",mob,NULL,NULL,TO_ROOM);
    
    if ( sn == -1 )
        for ( spell = 0; arcane_cost[spell].name != NULL; spell++ )
        {
            sn = skill_lookup( arcane_cost[spell].name );
            if ( sn == -1 )
            {
                bugf( "do_spellup: spell not found: %s", arcane_cost[spell].name );
                return;
            }
            skill_table[sn].spell_fun(sn, 90, mob, ch, TARGET_CHAR, FALSE);
        }
    else
        skill_table[sn].spell_fun(sn, 90, mob, ch, TARGET_CHAR, FALSE);
}
