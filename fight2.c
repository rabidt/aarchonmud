#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "mob_stats.h"

bool  check_lose_stance args( (CHAR_DATA *ch) );
bool  can_steal     args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, bool verbose ) );
bool  check_jam( CHAR_DATA *ch, int odds, bool offhand );

DECLARE_DO_FUN( do_disarm_trap );
DECLARE_DO_FUN( do_ignite );
DECLARE_DO_FUN( do_hide );

/*
* Disarm a creature.
* Caller must check for successful attack.
*/
bool disarm( CHAR_DATA *ch, CHAR_DATA *victim, bool quiet, int attack_mastery )
{
    OBJ_DATA *obj;
    
    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
        return FALSE;
    
    if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE))
    {
        if ( !quiet )
        {
            act("$S weapon won't budge!", ch, NULL, victim, TO_CHAR);
            act("$n tries to disarm you, but your weapon won't budge!", ch, NULL, victim, TO_VICT);
            act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT);
        }
        return FALSE;
    }
    
    int mastery = get_mastery(victim, get_weapon_sn(victim)) - attack_mastery;
    if ( per_chance(mastery == 2 ? 50 : mastery == 1 ? 30 : 0) )
    {
        if ( !quiet )
        {
            act("$N maintains a tight grip on $S weapon!", ch, NULL, victim, TO_CHAR);
            act("$n tries to disarm you, but you maintain your grip!", ch, NULL, victim, TO_VICT);
            act("$n tries to disarm $N, but $N maintains $S grip.", ch, NULL, victim, TO_NOTVICT);
        }
        return FALSE;
    }
    
    if (!quiet)
    {
        act( "$n DISARMS you and sends your weapon flying!", ch, NULL, victim, TO_VICT );
        act( "You disarm $N!",  ch, NULL, victim, TO_CHAR    );
        act( "$n disarms $N!",  ch, NULL, victim, TO_NOTVICT );
    }
    
    // disarm leaves weapon in inventory to prevent item loss, but marked as disarmed.
    unequip_char(victim, obj);
    SET_BIT(obj->extra_flags, ITEM_DISARMED);
    /*
    obj_from_char( obj );
    if ( IS_OBJ_STAT(obj,ITEM_NODROP) 
	 || IS_OBJ_STAT(obj,ITEM_INVENTORY)
	 || IS_OBJ_STAT(obj, ITEM_STICKY)
	 || IS_SET(victim->in_room->room_flags, ROOM_ARENA)
	 || (PLR_ACT(victim, PLR_WAR)) )
        obj_to_char( obj, victim );
    else
    {
        obj_to_room( obj, victim->in_room );
        if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,obj))
            get_obj(victim,obj,NULL);        
    }
    */
    
    return TRUE;
}

DEF_DO_FUN(do_berserk)
{
    int chance, hp_percent;
    int cost = 100;
    
    if ((chance = get_skill(ch,gsn_berserk)) == 0
        ||  (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BERSERK)))
    {
        send_to_char("You turn red in the face, but nothing happens.\n\r",ch);
        return;
    }
    
    if (IS_AFFECTED(ch,AFF_BERSERK) || is_affected(ch,gsn_berserk)
        ||  is_affected(ch,skill_lookup("frenzy")))
    {
        send_to_char("You get a little madder.\n\r",ch);
        return;
    }
    
    if (IS_AFFECTED(ch,AFF_CALM))
    {
        send_to_char("You're feeling to mellow to berserk.\n\r",ch);
        return;
    }
    
    if (ch->move < cost)
    {
        send_to_char("You can't get up enough energy.\n\r",ch);
        return;
    }
    
    /* modifiers */
    
    /* fighting */
    if (ch->position == POS_FIGHTING)
        chance += 10;
    
    /* damage -- being hurt helps */
    hp_percent = 100 * ch->hit/ch->max_hit;
    chance += (100 - hp_percent) / 5;
    
    if ( per_chance(chance) )
    {
        AFFECT_DATA af;
        
        WAIT_STATE(ch, skill_table[gsn_berserk].beats);
        ch->move -= cost;
        
        /* heal a little damage */
        gain_hit(ch, ch->level * 2);
        
        send_to_char("Your pulse races as you are consumed by rage!\n\r",ch);
        act("$n gets a wild look in $s eyes.",ch,NULL,NULL,TO_ROOM);
        check_improve(ch,gsn_berserk,TRUE,3);

        af.where    = TO_AFFECTS;
        af.type     = gsn_berserk;
        af.level    = ch->level;
        af.duration = get_duration(gsn_berserk, ch->level);
        af.modifier = UMAX(1,ch->level/5) + mastery_bonus(ch, gsn_berserk, 6, 10);
        af.bitvector    = AFF_BERSERK;
    
        af.location = APPLY_HITROLL;
        affect_to_char(ch,&af);
    
        af.location = APPLY_DAMROLL;
        affect_to_char(ch,&af);
    
        af.modifier = 10 * UMAX(1, ch->level/10);
        af.location = APPLY_AC;
        affect_to_char(ch,&af);
    }
    
    else
    {
        WAIT_STATE(ch, 2*skill_table[gsn_berserk].beats);
        ch->move -= cost/2;
        
        send_to_char("Your pulse speeds up, but nothing happens.\n\r",ch);
        check_improve(ch,gsn_berserk,FALSE,3);
    }
}

// combined bash + shield bash + charge routine
static void bash_char(CHAR_DATA *ch, const char *argument, int sn)
{
    CHAR_DATA *victim;
    const char *action = (sn == gsn_charge ? "charge" : sn == gsn_shield_bash ? "shield bash" : "bash");
    int skill = get_skill(ch, sn);
    
    if ( skill == 0 )
    {
        ptc(ch, "You don't know how to %s.\n\r", action);
        return;
    }
    
    if ( sn != gsn_bash && !get_eq_char(ch, WEAR_SHIELD) )
    {
        ptc(ch, "You need to wear a shield to %s.\n\r", action);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( victim->position < POS_FIGHTING )
    {
        act("You'll have to let $M get back up first.", ch, NULL, victim, TO_CHAR);
        return;
    } 

    if ( victim == ch )
    {
        send_to_char("You try to bash your brains out, but fail.\n\r", ch);
        return;
    }
    
    check_killer(ch, victim);
    WAIT_STATE(ch, skill_table[sn].beats);
    
    /* check whether a blow hits */
    int chance_hit = dodge_adjust_chance(ch, victim, skill);
    
    if ( !per_chance(chance_hit) )
    {
        damage(ch, victim, 0, sn, DAM_BASH, FALSE);
        act("You fall flat on your face!", ch, NULL, victim, TO_CHAR);
        act("$n falls flat on $s face.", ch, NULL, victim, TO_NOTVICT);
        act("You evade $n's $t, causing $m to fall flat on $s face.", ch, action, victim, TO_VICT);
        check_improve(ch, sn, FALSE, 3);
        if ( ch->stance != STANCE_RHINO )
            check_lose_stance(ch);
        set_pos(ch, POS_RESTING);
        return;
    }
    
    bash_effect(ch, victim, sn);
}

void bash_effect( CHAR_DATA *ch, CHAR_DATA *victim, int sn )
{
    // bash < shield bash < charge, and non-wrist shield helps too
    int power = (sn == gsn_charge ? 4 : sn == gsn_shield_bash ? 2 : 1);
    if ( sn != gsn_bash && !offhand_occupied(ch) )
        power += 1;
    
    /* check if the attack stuns the opponent */
    if ( !IS_AFFECTED(victim, AFF_ROOTS) && combat_maneuver_check(ch, victim, sn, STAT_STR, STAT_CON, 50 + 5 * power) )
    {
        if ( sn == gsn_charge )
        {
            act("$n charges into you, sending you sprawling!", ch, NULL, victim, TO_VICT);
            act("You charge into $N, and send $M sprawling!", ch, NULL, victim, TO_CHAR);
            act("$n sends $N sprawling with a powerful charge.", ch, NULL, victim, TO_NOTVICT);
            WAIT_STATE(victim, skill_table[sn].beats);
            victim->stop += 1;
        }
        else if ( sn == gsn_shield_bash )
        {
            act("$n bashes you in the face with his shield, sending you sprawling!", ch, NULL, victim, TO_VICT);
            act("You slam your shield into $N, and send $M sprawling!", ch, NULL, victim, TO_CHAR);
            act("$n sends $N sprawling with a well placed shield to the face.", ch, NULL, victim, TO_NOTVICT);
        }
        else
        {
            act("$n sends you sprawling with a powerful bash!", ch, NULL, victim, TO_VICT);
            act("You slam into $N, and send $M sprawling!", ch, NULL, victim, TO_CHAR);
            act("$n sends $N sprawling with a powerful bash.", ch, NULL, victim, TO_NOTVICT);
        }
        DAZE_STATE(victim, skill_table[sn].beats * 3/2);
        destance(victim, get_mastery(ch, sn));
        set_pos(victim, POS_RESTING);
    }
    else /* hit but no stun */
    {
        act("You slam into $N, but to no effect!", ch, NULL, victim, TO_CHAR);
        act("$n slams into $N, who stands like a rock!", ch, NULL, victim, TO_NOTVICT);
        act("You withstand $n's $t with ease.", ch, skill_table[sn].name, victim, TO_VICT);
        if ( ch->stance != STANCE_RHINO )
            check_lose_stance(ch);
    }
    check_improve(ch, sn, TRUE, 3);

    /* deal damage */
    int dam = martial_damage(ch, victim, sn) * power / 2;
    if ( sn == gsn_bash && IS_SET(ch->parts, PART_TUSKS) )
        full_dam(ch, victim, dam * 3/2, sn, DAM_PIERCE, TRUE);
    else
        full_dam(ch, victim, dam, sn, DAM_BASH, TRUE);
}

DEF_DO_FUN(do_bash)
{
    bash_char(ch, argument, gsn_bash);
}

DEF_DO_FUN(do_shield_bash)
{
    bash_char(ch, argument, gsn_shield_bash);
}

DEF_DO_FUN(do_charge)
{
    bash_char(ch, argument, gsn_charge);
}

DEF_DO_FUN(do_dirt)
{
    CHAR_DATA *victim;
    int skill, chance;
    
    if ( (skill = get_skill(ch,gsn_dirt)) == 0
        ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK_DIRT)))
    {
        send_to_char("You get your feet dirty.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;

    if (IS_AFFECTED(victim,AFF_BLIND))
    {
        act("$E's already been blinded.",ch,NULL,victim,TO_CHAR);
        return;
    }
    
    if (victim == ch)
    {
        send_to_char("Very funny.\n\r",ch);
        return;
    }
    
    /* terrain determines base chance */
    chance = 50;
    switch(ch->in_room->sector_type)
    {
    case(SECT_INSIDE):      chance -= 20;   break;
    case(SECT_CITY):        chance -= 10;   break;
    case(SECT_FIELD):       chance +=  5;   break;
    case(SECT_FOREST):              break;
    case(SECT_HILLS):               break;
    case(SECT_MOUNTAIN):            break;
    case(SECT_WATER_SHALLOW):
    case(SECT_WATER_DEEP):
    case(SECT_UNDERWATER):  chance  =  0;   break;
    case(SECT_AIR):         chance  =  0;   break;
    case(SECT_DESERT):      chance += 10;   break;
    }
    
    if (chance == 0)
    {
        send_to_char("There isn't any dirt to kick.\n\r",ch);
        return;
    }
    
    check_killer(ch,victim);
    WAIT_STATE(ch, skill_table[gsn_dirt].beats);
    start_combat(ch, victim);
    
    /* now the attack */
    if ( per_chance(skill) && combat_maneuver_check(ch, victim, gsn_dirt, STAT_DEX, STAT_AGI, chance) )
    {
        AFFECT_DATA af;
        act("$n is blinded by the dirt in $s eyes!",victim,NULL,NULL,TO_ROOM);
        act("$n kicks dirt in your eyes!",ch,NULL,victim,TO_VICT);
        send_to_char("You can't see a thing!\n\r",victim);
        check_improve(ch,gsn_dirt,TRUE,3);
        
        af.where    = TO_AFFECTS;
        af.type     = gsn_dirt;
        af.level    = ch->level;
        af.duration = 0;
        af.location = APPLY_HITROLL;
        af.modifier = -4;
        af.bitvector    = AFF_BLIND;
        
        affect_to_char(victim,&af);
    }
    else
    {
        damage(ch,victim,0,gsn_dirt,DAM_NONE,TRUE);
        check_improve(ch,gsn_dirt,FALSE,3);
    }
}

DEF_DO_FUN(do_trip)
{
    CHAR_DATA *victim;
    int skill = get_skill(ch,gsn_trip);
    
    if ( skill == 0 )
    {
        send_to_char("Tripping?  Like with mushrooms?\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( IS_AFFECTED(victim, AFF_ROOTS) )
    {
        act("$N is rooted firmly to the ground.", ch, NULL, victim, TO_CHAR);
        return;
    }
    
    if ( IS_AFFECTED(victim,AFF_FLYING) && !IS_AFFECTED(ch,AFF_FLYING))
    {
        act("$S feet aren't on the ground and yours are.",ch,NULL,victim,TO_CHAR);
        return;
    }
    
    if ( victim->position < POS_FIGHTING )
    {
        act("$N is already down.",ch,NULL,victim,TO_CHAR);
        return;
    }
    
    if ( victim == ch )
    {
        send_to_char("You fall flat on your face!\n\r", ch);
        act("$n trips over $s own feet!", ch, NULL, NULL, TO_ROOM);
        set_pos(ch, POS_RESTING);
        return;
    }
    
    /* now the attack */
    check_killer(ch,victim);
    WAIT_STATE(ch,skill_table[gsn_trip].beats);
    
    if ( per_chance(skill) && combat_maneuver_check(ch, victim, gsn_trip, STAT_DEX, STAT_AGI, 50) )
    {
        act("$n trips you and you go down!", ch, NULL, victim, TO_VICT);
        act("You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR);
        act("$n trips $N, sending $M to the ground.", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch, gsn_trip, TRUE, 3);
        
        destance(victim, get_mastery(ch, gsn_trip));
        DAZE_STATE(victim, skill_table[gsn_trip].beats);
        set_pos(victim, POS_RESTING);

        int dam = martial_damage(ch, victim, gsn_trip) * (3 + victim->size) / 10;
        full_dam(ch, victim, dam, gsn_trip, DAM_BASH, TRUE);
    }
    else
    {
        damage(ch, victim, 0, gsn_trip, DAM_BASH, TRUE);
        check_improve(ch, gsn_trip, FALSE, 3);
    } 
}

DEF_DO_FUN(do_backstab)
{
    CHAR_DATA *victim;
    int hips_skill = get_hips_skill(ch);

    if ( ch->fighting && !hips_skill )
    {
        send_position_message(ch);
        return;
    }
    
    if ( !ch->fighting && argument[0] == '\0' )
    {
        send_to_char("Backstab whom?\n\r",ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( !can_see(ch, victim) )
    {
        send_to_char("You can't see your target.\n\r", ch);
        return;
    }

    if ( victim == ch )
    {
        send_to_char( "How can you sneak up on yourself?\n\r", ch );
        return;
    }

    // hips + instant backstab
    if ( ch->fighting )
    {
        do_hide(ch, "");
        if ( ch->fighting )
            return;
    }
    
    backstab_char( ch, victim );
}

void backstab_char( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int chance;

    if ( (chance = get_skill(ch, gsn_backstab)) < 1 )
    {
        send_to_char("You dirty rat. Better learn how to do it properly first.\n\r", ch);
        return;
    }
    
    if ( check_see_combat(victim, ch) )
    {
        chance += (get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_AGI)) / 8;
        chance -= 75 - mastery_bonus(ch, gsn_backstab, 20, 25);
    }
    
    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_backstab].beats );
    if ( per_chance(chance) || (chance >= 2 && !IS_AWAKE(victim)) )
    {
        OBJ_DATA *weapon = get_eq_char(ch, WEAR_WIELD);
        OBJ_DATA *offhand = get_eq_char(ch, WEAR_SECONDARY);
        
        check_improve(ch,gsn_backstab,TRUE,2);

        if ( one_hit(ch, victim, gsn_backstab, FALSE) )
        {
            check_assassinate(ch, victim, weapon, 5);
            CHECK_RETURN(ch, victim);
        }

        if ( offhand_attack_chance(ch, TRUE) && one_hit(ch, victim, gsn_backstab, TRUE) )
        {
            check_assassinate(ch, victim, offhand, 5);
            CHECK_RETURN(ch, victim);
        }
    }
    else
    {
        act( "You failed to sneak up on $N.", ch, NULL, victim, TO_CHAR );
        damage( ch, victim, 0, gsn_backstab,DAM_NONE,TRUE);
        check_improve(ch,gsn_backstab,FALSE,2);
    }
}

DEF_DO_FUN(do_headbutt)
{
    CHAR_DATA *victim;
    int chance, dam;
    char arg[MAX_INPUT_LENGTH];
    
    one_argument(argument, arg);
    
    chance = get_skill(ch, gsn_headbutt);
    if ( IS_SET(ch->parts, PART_HORNS) )
	chance = (chance + 100) / 2;

    if (chance == 0)
    {
        send_to_char("Your skull isn't thick enough.\n\r", ch );
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    check_killer(ch,victim);
    WAIT_STATE( ch, skill_table[gsn_headbutt].beats );
    if ( check_hit(ch, victim, gsn_headbutt, DAM_BASH, chance) )
    {
        int dam_type = DAM_BASH;
        dam = one_hit_damage( ch, victim, gsn_headbutt, NULL ) * 3;

        if( IS_SET(ch->parts, PART_HORNS) )
        {
            dam += dam / 5;
            dam_type = DAM_PIERCE;
        }
        else if ( number_bits(2) == 0 )
        {
            send_to_char( "You suffer from brain damage. Ouch!\n\r", ch );
            ch->mana = UMAX(0, ch->mana - dam/3);
        }

        full_dam(ch,victim, dam, gsn_headbutt,dam_type,TRUE);
        check_improve(ch,gsn_headbutt,TRUE,3);
    }
    else
    {
        damage( ch, victim, 0, gsn_headbutt,DAM_BASH,TRUE);
        check_improve(ch,gsn_headbutt,FALSE,3);
    }
    return;
}

DEF_DO_FUN(do_net)
{
    CHAR_DATA *victim;
    int skill;
    
    if ( (skill = get_skill(ch,gsn_net)) == 0 /*|| IS_NPC(ch)*/ )
    {
        send_to_char("You wonder where you could get a net.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( !can_see_combat( ch, victim) )
    {
        send_to_char( "What? Where? You can't net someone you don't see!\n\r", ch );
        return;
    }

    if ( is_affected(victim, gsn_net) )
    {
        act("$E is already tied up.",ch,NULL,victim,TO_CHAR);
        return;
    }
    
    if ( victim == ch )
    {
        send_to_char("Very funny.\n\r",ch);
        return;
    }
    
    check_killer(ch,victim);
    WAIT_STATE(ch, skill_table[gsn_net].beats);
    start_combat(ch, victim);
    
    /* now the attack */
    if ( per_chance(skill) && combat_maneuver_check(ch, victim, gsn_net, STAT_DEX, STAT_AGI, 33) )
    {
        AFFECT_DATA af;
        act("$N is trapped in your net!",ch, NULL, victim, TO_CHAR);
        act("$n traps $N in a net!",ch, NULL, victim, TO_ROOM);
        act("$n entraps you in a net!",ch,NULL,victim,TO_VICT);
        send_to_char("You stumble around in the net!\n\r",victim);
        check_improve(ch,gsn_net,TRUE,3);
        WAIT_STATE(victim, skill_table[gsn_net].beats);
        
        af.where    = TO_AFFECTS;
        af.type     = gsn_net;
        af.level    = ch->level;
        af.duration = 1;
        af.location = APPLY_DEX;
        af.modifier = -ch->level/8;
        af.bitvector = AFF_SLOW;
        
        affect_to_char(victim,&af);
    }
    else
    {
        damage(ch,victim,0,gsn_net,DAM_NONE,TRUE);
        check_improve(ch,gsn_net,FALSE,3);
    }
}

// mama routine for burst, semi-auto and full-auto
void spray_attack( CHAR_DATA *ch, const char *argument, int sn )
{
    CHAR_DATA *victim, *target, *next_target;
    OBJ_DATA *first, *second;
    bool secondgun = FALSE;
    int targeted_attacks, area_attacks, jam_chance;
    int skill = get_skill(ch, sn);

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;

    first = get_eq_char(ch, WEAR_WIELD);
    second = get_eq_char(ch, WEAR_SECONDARY);
    
    // do we have gun(s) to use?
    if ( !first || first->value[0] != WEAPON_GUN )
    {
        send_to_char( "You need a gun for that.\n\r", ch );
        return;
    }
    if ( IS_SET(first->extra_flags, ITEM_JAMMED) )
    {
        send_to_char( "Not with a jammed gun.\n\r", ch );
        return;
    }
    if ( second && second->value[0] == WEAPON_GUN && !IS_SET(second->extra_flags, ITEM_JAMMED) )
        secondgun = TRUE;
    
    // ok, we're ready for action
    WAIT_STATE( ch, skill_table[sn].beats );

    if ( !per_chance(skill) && per_chance(5) )
    {
        send_to_char("You shoot yourself in the foot!\n\r", ch);
        act( "$n shoots $mself in the foot!", ch, NULL, NULL, TO_ROOM);
        damage(ch, ch, one_hit_damage(ch, ch, sn, first), sn, get_weapon_damtype(first), TRUE);
        check_improve(ch, sn, FALSE, 3);
        return;
    }

    check_improve(ch, sn, TRUE, 3);
    
    // work out number of attacks & chance of jamming
    targeted_attacks = number_range(2,4);
    area_attacks = (sn == gsn_fullauto ? number_range(2,targeted_attacks) : sn == gsn_semiauto ? number_range(1,2) : 0);
    if ( per_chance(get_skill(ch, gsn_tight_grouping)) )
    {
        // plus 1/3 to number of attacks
        targeted_attacks = rand_div(targeted_attacks * 4, 3);
        area_attacks = rand_div(area_attacks * 4, 3);
        check_improve(ch, gsn_tight_grouping, TRUE, 4);
    }
    jam_chance = (sn == gsn_fullauto ? 4 : sn == gsn_semiauto ? 2 : 1);

    // now the attacks
    for ( target = ch->in_room->people; target; target = next_target )
    {
        next_target = target->next_in_room;
        
        if ( target != victim && is_safe_spell(ch, target, TRUE) )
            continue;

        int attacks = (target == victim ? targeted_attacks : area_attacks);
        while ( attacks-- > 0 )
        {
            one_hit( ch, target, sn, FALSE);
            if ( check_jam(ch, jam_chance, FALSE) )
                return;
            
            if ( secondgun && per_chance(33) )
            {
                one_hit(ch, target, sn, TRUE);
                if ( check_jam(ch, jam_chance, TRUE) )
                    secondgun = false;
            }
        }
    }
}

DEF_DO_FUN(do_burst)
{
    spray_attack(ch, argument, gsn_burst);
}

DEF_DO_FUN(do_fullauto)
{
    spray_attack(ch, argument, gsn_fullauto);
}

DEF_DO_FUN(do_semiauto)
{
    spray_attack(ch, argument, gsn_semiauto);
}

DEF_DO_FUN(do_hogtie)
{
    CHAR_DATA *victim;
    OBJ_DATA *rope;
    int skill;
    AFFECT_DATA af;
    
    if ( (skill = get_skill(ch, gsn_hogtie)) == 0 )
    {
        send_to_char("Better leave that to the hogtyin' professionals.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( victim == ch )
    {
        send_to_char("Sick, sick, sick.\n\r",ch);
        return;
    }
    
    rope = get_eq_char(ch, WEAR_HOLD);
    if ((rope == NULL) || (rope->item_type != ITEM_HOGTIE))
    {
        send_to_char("You'll need some good hogtyin' rope in hand.\n\r",ch);
        return;
    }
    
    if ( is_affected(victim, gsn_hogtie) )
    {
        act( "$N is already tied up.",ch,NULL,victim,TO_CHAR);
        return;
    }

    check_killer(ch,victim);
    WAIT_STATE(ch,skill_table[gsn_hogtie].beats);
    start_combat(ch, victim);
    
    /* now the attack */
    if ( per_chance(skill) && combat_maneuver_check(ch, victim, gsn_hogtie, STAT_DEX, STAT_AGI, 50) )
    {
	//extract_obj( rope );
        act("You hogtie $N!",ch, NULL, victim, TO_CHAR);
        act("$n hogties $N, and do they look pissed!",ch, NULL, victim, TO_NOTVICT);
        act("$n hogties you! How embarassing!",ch,NULL,victim,TO_VICT);
        send_to_char("You try to get out of the hogtie!\n\r",victim);
        check_improve(ch,gsn_hogtie,TRUE,2);
        WAIT_STATE(ch,skill_table[gsn_hogtie].beats);
        WAIT_STATE(victim, skill_table[gsn_hogtie].beats);
        destance(victim, get_mastery(ch, gsn_hogtie));
        
        af.where    = TO_AFFECTS;
        af.type     = gsn_hogtie;
        af.level    = ch->level;
        af.duration = get_duration(gsn_hogtie, ch->level);
        af.location = APPLY_AGI;
        af.modifier = -20;
        af.bitvector = AFF_SLOW;
        
        affect_to_char(victim,&af);
    }
    else
    {
        send_to_char ("Your wily opponent evades your attempts to hogtie.\n\r", ch);
        act("$n attempts to hogtie you, but you twist out of the way.",ch,NULL,victim,TO_VICT);
        check_improve(ch,gsn_hogtie,FALSE,2);
        WAIT_STATE(ch,skill_table[gsn_hogtie].beats);
    }
}

// checks whether character can let off a shot, returning accuracy < 100 if using offhand weapon
// accuracy of 0 means can't shoot, error message has been sent
int get_shot_accuracy( CHAR_DATA *ch )
{
    OBJ_DATA *wield = get_eq_char(ch, WEAR_WIELD);
    OBJ_DATA *offhand = get_eq_char(ch, WEAR_SECONDARY);
    
    if ( !is_ranged_weapon(wield) && !is_ranged_weapon(offhand) )
    {    
        send_to_char( "You need to wield a ranged weapon for that.\n\r", ch );
        return 0;
    }

    if ( wield && wield->value[0] == WEAPON_BOW )
    {
        OBJ_DATA *held = get_eq_char(ch, WEAR_HOLD);
        if ( !held || held->item_type != ITEM_ARROWS )
        {
            send_to_char( "Without arrows that's not going to work.\n\r", ch );
            return 0;
        }
        return 100;
    }
    
    if ( wield && wield->value[0] == WEAPON_GUN && !IS_SET(wield->extra_flags, ITEM_JAMMED) )
        return 100;
    
    // last chance, try offhand - accuracy is capped at 90%
    if ( offhand && offhand->value[0] == WEAPON_GUN && !IS_SET(offhand->extra_flags, ITEM_JAMMED) )
        return 50 + offhand_attack_chance(ch, FALSE) * 2/5;
        
    // at this point we must have a gun, and all guns must be jammed
    send_to_char("Not with a jammed gun.\n\r", ch);
    return 0;
}

/* parameters for aiming, must terminate with "" */
const char* aim_targets[] = { "head", "hand", "foot", "" };
/* constants must be defined according to aim_targets */
#define AIM_NORMAL -1
#define AIM_HEAD    0
#define AIM_HAND    1
#define AIM_FOOT    2

DEF_DO_FUN(do_aim)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj; 
    int i, chance, accuracy;
    int aim_target = AIM_NORMAL;
    bool secondgun = FALSE;

    argument = one_argument( argument, arg );
        
    if ((get_skill(ch,gsn_aim)) == 0)
    {
        send_to_char( "You haven't the foggiest idea how.\n\r", ch );
        return; 
    }
    
    for (i = 0; aim_targets[i][0]; i++)
                if (!strcmp(arg, aim_targets[i]))
                {
                        aim_target = i;
                        argument = one_argument(argument, arg);
                        break;
                }

    if ( (victim = get_combat_victim(ch, arg)) == NULL )
        return;
        
    if (victim == ch)
    {
        send_to_char("You aim at yourself, but come to your senses just in time.\n\r",ch);
        return; 
    }
        
    if ( !can_see_combat(ch, victim) )
    {
        send_to_char("You don't see your target, how can you aim?\n\r", ch );
        return;
    }

    if ( is_safe(ch,victim) )
        return;

    if ( (accuracy = get_shot_accuracy(ch)) == 0 )
        return;
    
    secondgun = accuracy < 100;
    
    check_killer( ch, victim );
    
    mastery_adjusted_wait(ch, gsn_aim);
    if ( aim_target == AIM_NORMAL )
        ch->wait = ch->wait * 2/3;
    
    chance = (100 + get_skill(ch, gsn_aim)) * accuracy / 200;

    if ( !IS_AWAKE(victim) )
        chance = (100 + chance) / 2;
    
    if ( per_chance(chance) )
    {
        if ( one_hit(ch, victim, gsn_aim, secondgun) )
        {
            switch (aim_target)
            {
            case AIM_NORMAL:
                break;
            case AIM_HEAD:
                obj = secondgun ? get_eq_char(ch, WEAR_SECONDARY) : get_eq_char(ch, WEAR_WIELD);
                check_assassinate(ch, victim, obj, 5);
                break;
            case AIM_HAND:
                if ( number_percent() < 50 )
                    disarm(ch, victim, FALSE, get_mastery(ch, gsn_aim));
                break;
            case AIM_FOOT:
                if ( get_weapon_sn(ch) == gsn_bow )
                {
                    act("Your arrow hits $N in the foot, making $M hop around for a few moments.", ch, NULL, victim, TO_CHAR);
                    act("$n's arrow hits you in the foot!!  The pain makes it difficult to stand on it.", ch, NULL, victim, TO_VICT);
                    act("$n's arrow hits $N in the foot, making $M hop around for a few moments.", ch, NULL, victim, TO_NOTVICT);
                }
                else
                {
                    act("Your bullet hits $N in the foot, making $M hop around for a few moments.", ch, NULL, victim, TO_CHAR);
                    act("$n's bullet hits you in the foot!!  The pain makes it difficult to stand on it.", ch, NULL, victim, TO_VICT);
                    act("$n's bullet hits $N in the foot, making $M hop around for a few moments.", ch, NULL, victim, TO_NOTVICT);
                }
                WAIT_STATE( victim, skill_table[gsn_aim].beats / 2 );
                victim->slow_move = UMAX(ch->slow_move, PULSE_VIOLENCE * 6);
                if( number_bits(1) )
                    destance(victim, get_mastery(ch, gsn_aim));
                break;
            default:
                bug("AIM: invalid aim_target: %d", aim_target);
            }
        }
        check_jam(ch, 1, secondgun);
        check_improve(ch, gsn_aim, TRUE, 3);
    }
    else
    {
        damage( ch, victim, 0, gsn_aim,DAM_NONE,TRUE);
        check_improve(ch, gsn_aim, FALSE, 3);
    }
        
    return;
}


DEF_DO_FUN(do_drunken_fury)
{
    int chance, hp_percent;
    int cost = 100;
    
    if ((chance = get_skill(ch,gsn_drunken_fury)) < 1)
    {
        send_to_char("You turn a little red.\n\r",ch);
        return;
    }
    
    if (IS_HERO(ch))
        send_to_char("At your level, You only need to remember being drunk!\n\r", ch);
    else
    {
        if (ch->pcdata->condition[COND_DRUNK] < 10 )
        {
            send_to_char( "You gotta be drunk for drunken fury.\n\r", ch );
            return;
        }
    }
    
    if (IS_AFFECTED(ch,AFF_BERSERK) || is_affected(ch,gsn_berserk)
        ||  is_affected(ch,skill_lookup("frenzy")))
    {
        send_to_char("You're already in a wild frenzy.\n\r",ch);
        return;
    }   
    
    if (IS_AFFECTED(ch,AFF_CALM))
    {
        send_to_char("You're feeling to mellow to be furious.\n\r",ch);
        return;
    }
    
    if (ch->move < cost)
    {
        send_to_char("You can't muster the fury.\n\r",ch);
        return;
    }
    
    /* modifiers */
    
    /* fighting */
    if (ch->position == POS_FIGHTING)
        chance += 10;
    
    /* damage -- being hurt helps */
    hp_percent = 100 * ch->hit/ch->max_hit;
    chance += (100 - hp_percent) / 5;
    
    if (number_percent() < chance)
    {
        AFFECT_DATA af;
        
        WAIT_STATE(ch, skill_table[gsn_drunken_fury].beats);
        ch->move -= cost;
        
        /* heal a little damage */
        gain_hit(ch, ch->level);
        
        send_to_char("You're drunk and furious!\n\r",ch);
        act("Look out world! $n is drunk and furious!",ch,NULL,NULL,TO_ROOM);
        check_improve(ch,gsn_drunken_fury,TRUE,3);
        
        af.where    = TO_AFFECTS;
        af.type     = gsn_drunken_fury;
        af.level    = ch->level;
        af.duration = (get_duration(gsn_drunken_fury, ch->level) + ch->pcdata->condition[COND_DRUNK]) / 2;
        af.modifier = UMAX(1,ch->level/3);
        af.bitvector    = AFF_BERSERK;
        
        af.location = APPLY_HITROLL;
        affect_to_char(ch,&af);
        
        af.location = APPLY_DAMROLL;
        affect_to_char(ch,&af);
        
        af.modifier = UMAX(10,10 * (ch->level/6));
        af.location = APPLY_AC;
        affect_to_char(ch,&af);
    }
    
    else
    {
        WAIT_STATE(ch, 2*skill_table[gsn_drunken_fury].beats);
        ch->move -= cost/2;
        
        send_to_char("You're certainly drunk, but not really furious.\n\r",ch);
        check_improve(ch,gsn_drunken_fury,FALSE,3);
    }
}


DEF_DO_FUN(do_snipe)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    
    if ( get_skill(ch,gsn_snipe) < 1)
    {
        send_to_char("You don't know how to snipe.\n\r",ch);
        return;
    }
    
    one_argument( argument, arg );
        
    if (arg[0] == '\0')
    {
        send_to_char("Snipe whom?\n\r",ch);
        return;
    }
     
    if (ch->fighting)
    {
        send_to_char("I think your opponent sees you.\n\r",ch);
        return;
    } 
     
    else if ((victim = get_victim_room(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }

    snipe_char(ch, victim);
}

void snipe_char( CHAR_DATA *ch, CHAR_DATA *victim )
{
    OBJ_DATA *obj;
    int skill = get_skill(ch, gsn_snipe);
    int accuracy;
    bool secondgun = FALSE;
        
    if ( victim == ch )
    {
        send_to_char( "You put your gun to your head, but come to your senses just in time.\n\r", ch );
        return;
    }
 
    if ( is_safe(ch,victim) )
        return;
     
    if ( (accuracy = get_shot_accuracy(ch)) == 0 )
        return;
    
    secondgun = accuracy < 100;
    skill = skill * accuracy / 100;

    check_killer( ch, victim );
    mastery_adjusted_wait(ch, gsn_snipe);

    if ( per_chance(skill) || !IS_AWAKE(victim) )
    {   
        if ( one_hit(ch, victim, gsn_snipe, secondgun) )
        {
            obj = secondgun ? get_eq_char(ch, WEAR_SECONDARY) : get_eq_char(ch, WEAR_WIELD);
            check_assassinate(ch, victim, obj, 5);
        }
        check_jam(ch, 2, secondgun);
        check_improve(ch,gsn_snipe,TRUE,2);
    }
    else
    {
        check_improve(ch,gsn_snipe,FALSE,2);
        damage( ch, victim, 0, gsn_snipe,DAM_NONE,TRUE);
    }
    return;
}

// uniform calculation for circle, slash throat, etc.
static int circle_chance( CHAR_DATA *ch, CHAR_DATA *victim, int sn )
{
    int chance = 10 + get_skill_total(ch, sn, 0.2) / 2 + mastery_bonus(ch, sn, 12, 15);
    chance += (get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_AGI)) / 8;
    // combat advantage
    if ( !can_see_combat(victim, ch) )
        chance += 10;
    if ( victim->position < POS_FIGHTING )
        chance += 10;
    if ( victim->fighting != ch )
        chance += 10;
    // haste/slow
    if ( IS_AFFECTED(ch, AFF_HASTE) )
        chance += 10;
    else if ( IS_AFFECTED(ch, AFF_SLOW) )
        chance -= 10;
    if ( IS_AFFECTED(victim, AFF_HASTE) )
        chance -= 10;
    else if ( IS_AFFECTED(victim, AFF_SLOW) )
        chance += 10;
    // heavy armor penalty for both attacker and victim
    chance += (get_heavy_armor_penalty(victim) - get_heavy_armor_penalty(ch)) / 10;
    return chance;
}

DEF_DO_FUN(do_circle)
{
    CHAR_DATA *victim;
    int chance;
    
    if ( (chance = get_skill(ch,gsn_circle)) < 1 )
    {
        send_to_char("You don't know how to circle.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( !can_see_combat(ch, victim) )
    {
        send_to_char("Circle whom?\n\r",ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char("You try to circle around yourself, and fail.\n\r",ch);
        return;
    }
    
    chance = circle_chance(ch, victim, gsn_circle);
    
    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_circle].beats );

    if ( per_chance(chance) )
    {
        OBJ_DATA *weapon = get_eq_char(ch, WEAR_WIELD);
        OBJ_DATA *offhand = get_eq_char(ch, WEAR_SECONDARY);
        
        check_improve(ch,gsn_circle,TRUE,3);

        if ( one_hit(ch, victim, gsn_circle, FALSE) )
            check_assassinate(ch, victim, weapon, 7);
        CHECK_RETURN(ch, victim);
        
        if ( offhand_attack_chance(ch, TRUE) && one_hit(ch, victim, gsn_circle, TRUE) )
        {
            check_assassinate(ch, victim, offhand, 7);
            CHECK_RETURN(ch, victim);
        }
    }
    else
    {
        act( "You fail to reach $N's back.", ch, NULL, victim, TO_CHAR );
        check_improve(ch,gsn_circle,FALSE,3);
        damage( ch, victim, 0, gsn_circle, DAM_NONE,TRUE);
    }
    
    return;
}


DEF_DO_FUN(do_slash_throat)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance;
    
    if ( (chance = get_skill(ch, gsn_slash_throat)) < 1 )
    {
        send_to_char("You don't know how to slash throats.\n\r",ch);
        return;
    }
    
    one_argument( argument, arg );
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( victim == ch )
    {
        send_to_char("Better not.\n\r",ch);
        return;
    }
    
    if ( !can_see_combat(ch, victim) )
    {
        send_to_char("You can't see them.\n\r",ch);
        return;
    }
    
    if ( (obj = get_eq_char(ch, WEAR_WIELD) ) == NULL )
    {
        if ( !IS_SET(ch->parts, PART_CLAWS) )
        {
            send_to_char( "You need to wield a weapon to slash throats.\n\r", ch );
            return;
        }
    }
    else
    {
        int base_dam = weapon_base_damage[obj->value[0]];
        if ( base_dam != DAM_SLASH && base_dam != DAM_PIERCE )
        {
            send_to_char( "You need an edged weapon to slash throats.\n\r", ch );
            return;
        }
    }

    if ( is_affected(victim, gsn_slash_throat) )
    {
        act( "$E already has enough trouble speaking.",ch,NULL,victim,TO_CHAR );
        return;
    }

    /* can be used like backstab OR like circle.. */
    if ( ch->fighting != NULL || check_see_combat(victim, ch) )
    {
        chance = circle_chance(ch, victim, gsn_slash_throat);
    }
    
    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_slash_throat].beats );

    if ( per_chance(chance) )
    {
        AFFECT_DATA af;

        check_improve(ch,gsn_slash_throat,TRUE,3);

        if ( !one_hit(ch, victim, gsn_slash_throat, FALSE) )
            return;
        CHECK_RETURN(ch, victim);

        check_assassinate(ch, victim, obj, 6);
        CHECK_RETURN(ch, victim);

        act("$n slashes your throat open!",ch,NULL,victim,TO_VICT);
        act("You slash cuts $N's throat open!",ch,NULL,victim,TO_CHAR);
        act("$n opens $N's throat with a well placed throat slash!",ch,NULL,victim,TO_NOTVICT);

        af.where    = TO_AFFECTS;
        af.type     = gsn_slash_throat;
        af.level    = ch->level;
        af.duration = 0;
        af.location = APPLY_VIT;
        af.modifier = -20;
        af.bitvector = 0;
        affect_to_char( victim, &af );
    }
    else
    {
        act( "You fail to reach $N's throat.", ch, NULL, victim, TO_CHAR );
        damage( ch, victim, 0, gsn_slash_throat, DAM_NONE,TRUE);
        check_improve(ch,gsn_slash_throat,FALSE,3);
    }
    
    return;
}


DEF_DO_FUN(do_rescue)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *fch;
    bool is_attacked = FALSE;
    int chance;
    
    one_argument( argument, arg );

    /*
    if ((chance = get_skill(ch,gsn_rescue)) < 1 && !IS_NPC(ch))
    {
        send_to_char("That's noble of you, but you don't know how.\n\r",ch);
        return;
    }
    */

    if ( arg[0] == '\0' )
    {
        send_to_char( "Rescue whom?\n\r", ch );
        return;
    }
    
    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
    
    if (!is_same_group(ch, victim) 
	&& !is_same_group(ch, victim->master) 
	&& victim->master != ch 
	&& ch->master != victim)
    {
        send_to_char("You must be grouped with them to rescue them.\n\r",ch);
        return;
    }
    
    if ( victim == ch )
    {
        send_to_char( "What about fleeing instead?\n\r", ch );
        return;
    }
    
    if ( ch->fighting == victim )
    {
        send_to_char( "Too late.\n\r", ch );
        return;
    }
    
    /* find character to rescue victim from */
    for ( fch = victim->in_room->people; fch != NULL; fch = fch->next_in_room )
    {        
        if ( fch->fighting == victim )
        {        
            is_attacked = TRUE;
            if ( !is_safe_spell(ch, fch, FALSE) )
                break;
        }
    }

    if ( fch == NULL )
    {
        if ( is_attacked )
            send_to_char( "You cannot interfere in this fight.\n\r",ch);
        else
            send_to_char( "That person isn't being attacked right now.\n\r", ch );
        return;
    }

    chance = 25 + get_skill(ch, gsn_rescue)/2 + get_skill(ch, gsn_bodyguard)/4;
    chance += (ch->level - victim->level) / 4;
    if (number_percent() < get_skill(fch, gsn_entrapment))
    {
      chance /= 5;
      check_improve(fch, gsn_entrapment, TRUE, 1);
    }

    WAIT_STATE( ch, skill_table[gsn_rescue].beats );
    if ( number_percent( ) > chance )
    {
        send_to_char( "You fail the rescue.\n\r", ch );
        check_improve(ch,gsn_rescue,FALSE,2);
        return;
    }
    
    act( "You rescue $N!",  ch, NULL, victim, TO_CHAR    );
    act( "$n rescues you!", ch, NULL, victim, TO_VICT    );
    act( "$n rescues $N!",  ch, NULL, victim, TO_NOTVICT );
    check_improve(ch,gsn_rescue,TRUE,2);
    
    /* removed to prevent kill-trigger to activate --Bobble
    stop_fighting( fch, FALSE );
    stop_fighting( victim, FALSE );
    */

    check_killer( ch, fch );
    if ( ch->fighting != fch )
        set_fighting( ch, fch );
    set_fighting( fch, ch );
    /*set_fighting( victim, other );*/
    return;
}

void mastery_adjusted_wait( CHAR_DATA *ch, int sn )
{
    int wait = skill_table[sn].beats;
    int bonus = mastery_bonus(ch, sn, 20, 25);
    WAIT_STATE( ch, rand_div(wait * (100 - bonus), 100) );
}

DEF_DO_FUN(do_kick)
{
    CHAR_DATA *victim;
    int dam, chance;
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;

    // anyone can kick
    chance = (100 + get_skill(ch, gsn_kick)) / 2;

    mastery_adjusted_wait(ch, gsn_kick);

    if ( check_hit(ch, victim, gsn_kick, DAM_BASH, chance) )
    {
        dam = martial_damage( ch, victim, gsn_kick );

        full_dam(ch,victim, dam, gsn_kick,DAM_BASH,TRUE);
        check_improve(ch,gsn_kick,TRUE,3);
    }
    else
    {
        damage( ch, victim, 0, gsn_kick,DAM_BASH,TRUE);
        check_improve(ch,gsn_kick,FALSE,3);
    }

    return;
}

DEF_DO_FUN(do_disarm)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int skill;
    char arg[MAX_INPUT_LENGTH];
    
    one_argument(argument,arg);
    
    // allow disarm as a shortcut for tdisarm while out-of-combat
    if ( !ch->fighting && !get_victim_room(ch, arg) && get_skill(ch, gsn_disarm_trap) > 0 )
    {
        do_disarm_trap( ch, argument );
        return;
    }
    
    if ((skill = get_skill(ch,gsn_disarm)) == 0)
    {
        send_to_char( "You don't know how to disarm opponents.\n\r", ch );
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;

    if ( !can_see_combat( ch, victim ) )
    {
        send_to_char( "You fumble for your opponent's weapon, but can't find it.\n\r", ch );
        return;
    }
    
    if ( (obj = get_eq_char(victim, WEAR_WIELD)) == NULL )
    {
        send_to_char( "Your opponent is not wielding a weapon.\n\r", ch );
        return;
    }
        
    // starting a fight if needed, regardless of success or failure
    check_killer(ch, victim);
    start_combat(ch, victim);
    WAIT_STATE( ch, skill_table[gsn_disarm].beats );

    /* whips disarm better, hand-to-hand worse */
    int chance = 50;
    if ( get_weapon_sn(ch) == gsn_hand_to_hand )
        chance -= 10;
    else if ( get_weapon_sn(ch) == gsn_whip )
        chance += 10;

    /* You no longer can fail disarm a bunch of times before finding
        out that your opponent's weapon is damned, if you have detect
        magic - Astark 6-8-13 */
    if ( IS_OBJ_STAT(obj, ITEM_NOREMOVE) && IS_AFFECTED(ch, AFF_DETECT_MAGIC) )
    {
        act("$S weapon won't budge!", ch, NULL, victim, TO_CHAR);
        act("$n tries to disarm you, but your weapon won't budge!", ch, NULL, victim, TO_VICT);
        act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT);
        return;
    }

    /* and now the attack */
    if ( per_chance(skill) && combat_maneuver_check(ch, victim, gsn_disarm, STAT_DEX, STAT_STR, chance) )
    {
        disarm(ch, victim, FALSE, get_mastery(ch, gsn_disarm));
        check_improve(ch, gsn_disarm, TRUE, 2);
    }
    else
    {
        act("You fail to disarm $N.", ch, NULL, victim, TO_CHAR);
        act("$n tries to disarm you, but fails.", ch, NULL, victim, TO_VICT);
        act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch, gsn_disarm, FALSE, 2);
    }
    return;
}

DEF_DO_FUN(do_surrender)
{
    CHAR_DATA *mob;

    if ( (mob = ch->fighting) == NULL )
    {
        send_to_char( "But you're not fighting!\n\r", ch );
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);
    act( "You surrender to $N!", ch, NULL, mob, TO_CHAR );
    act( "$n surrenders to you!", ch, NULL, mob, TO_VICT );
    act( "$n tries to surrender to $N!", ch, NULL, mob, TO_NOTVICT );
    stop_fighting( ch, FALSE );
    if ( mob->fighting == ch )
	stop_fighting( mob, FALSE );

    if ( PLR_ACT(mob, PLR_NOSURR)
	 || PLR_ACT(mob, PLR_WAR)
	 || (IS_NPC(mob) && !mp_percent_trigger(mob, ch, NULL,0, NULL,0, TRIG_SURR)) )
    {
	act( "$N seems to ignore your cowardly act!", ch, NULL, mob, TO_CHAR );
        multi_hit( mob, ch, TYPE_UNDEFINED );
    }
    else
    {
	forget_attacks(mob);
    }
}



void split_attack ( CHAR_DATA *ch, int dt )
{
    int     chance, attacks, opponents=0, duals=0;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    bool found=FALSE;
    
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if ( vch != ch && !is_safe_spell(ch,vch,TRUE)
            && (IS_NPC(vch) || vch==ch->fighting || is_same_group(vch, ch->fighting)))
            opponents++;
    }
    
    attacks=opponents;
    
    if (get_eq_char (ch, WEAR_SECONDARY))
    {
        chance = get_skill(ch,gsn_dual_wield);
        chance = UMAX(chance, get_skill(ch, gsn_dual_dagger));
        chance = UMAX(chance, get_skill(ch, gsn_dual_sword)); 
        chance += get_curr_stat(ch, STAT_DEX)/2;
        chance -= 40;
        
        if (number_percent() < chance)
        {
            if (get_skill(ch, gsn_dual_wield)==0)
                check_improve(ch,gsn_second_attack,TRUE,5);
            duals++;
            attacks++;
        }
        
        if (IS_AFFECTED(ch,AFF_HASTE))
        {
            chance -=40;
            if (number_percent() <chance)
            {
                duals++;
                attacks++;
            }
        }
    }
    
    
    if (IS_AFFECTED(ch,AFF_HASTE)) attacks +=1;
    if (IS_AFFECTED(ch,AFF_BERSERK)) attacks +=1;
    if (IS_AFFECTED(ch,AFF_SLOW)) attacks -=1;
    
    if (get_eq_char( ch, WEAR_WIELD ) == NULL) 
	if (number_percent() <get_skill (ch, gsn_kung_fu))
        attacks +=1;
    
    chance = (get_skill(ch,gsn_second_attack) +  get_curr_stat(ch, STAT_DEX)/4)/2;
    
    if ( number_percent( ) < chance )
    {
        attacks +=1;
        check_improve(ch,gsn_second_attack,TRUE,5);
    }
    
    chance = (get_skill(ch,gsn_third_attack) + get_curr_stat(ch, STAT_DEX)/4)/4;
    
    if ( number_percent( ) < chance ) 
    {
        attacks +=1;
        check_improve(ch,gsn_third_attack,TRUE,5);
    }
    
    do {
        found=FALSE;
        for ( vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (attacks == 0) break;
            if ( vch != ch && !is_safe_spell(ch,vch,TRUE)
                && (IS_NPC(vch) || vch==ch->fighting || is_same_group(vch, ch->fighting)))
            {
                attacks--;
                found=TRUE;
                if (duals == attacks)
                {
                    one_hit(ch, vch, dt, TRUE);
                    duals--;
                } else one_hit(ch, vch, dt, FALSE);
            }
        }
    } while ((attacks > 0)&&found);
    return;
    
    affect_strip(ch, gsn_melee);
    affect_strip(ch, gsn_brawl);
}



DEF_DO_FUN(do_gouge)
{
    CHAR_DATA *victim;
    int skill;
    
    if ( (skill = get_skill(ch,gsn_gouge)) == 0)
    {
        send_to_char("Gouging?  Sounds messy.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( !can_see_combat( ch, victim) )
    {
        send_to_char( "You can't find your opponent's eyes.\n\r", ch );
        return;
    }
    
    if ( is_affected(victim, gsn_gouge) /*IS_AFFECTED(victim,AFF_BLIND)*/)
    {
        act("$E's already been blinded.",ch,NULL,victim,TO_CHAR);
        return;
    }
    
    if (victim == ch)
    {
        send_to_char("You poke yourself in the eye.\n\r",ch);
        return;
    }
    
    if ( is_safe(ch,victim) )
        return;
    
    check_killer(ch,victim);
    WAIT_STATE(ch, skill_table[gsn_gouge].beats);
    
    /* now the attack */
    int chance = dodge_adjust_chance(ch, victim, skill);
    if ( per_chance(chance) )
    {
        int dam = martial_damage(ch, victim, gsn_gouge);
        full_dam(ch, victim, dam, gsn_gouge, DAM_BASH, TRUE);
        check_improve(ch, gsn_gouge, TRUE, 3);
    }
    else
    {
        full_dam(ch, victim, 0, gsn_gouge, DAM_BASH, TRUE);
        check_improve(ch, gsn_gouge, FALSE, 3);
        return;
    }

    /* if attack hits, we have a chance of permanent blindness */
    if ( combat_maneuver_check(ch, victim, gsn_gouge, STAT_STR, STAT_AGI, 20) )
    {
        AFFECT_DATA af;
        act("$n is blinded as $s eyes are gouged out!", victim, NULL, NULL, TO_ROOM);
        act("$n gouges your eyes out!", ch, NULL, victim, TO_VICT);
        send_to_char("You can't see a thing!\n\r", victim);
        
        af.where    = TO_AFFECTS;
        af.type     = gsn_gouge;
        af.level    = ch->level;
        af.duration = -1;
        af.location = APPLY_HITROLL;
        af.modifier = -4;
        af.bitvector = AFF_BLIND;
        
        affect_to_char(victim,&af);
    }
}

DEF_DO_FUN(do_leg_sweep)
{
    int tally = 0;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int skill;
    bool ch_is_flying = IS_AFFECTED(ch, AFF_FLYING) != 0;
    
    if ( IS_SET(ch->in_room->room_flags, ROOM_SAFE) )
    {
        send_to_char( "Not in this room.\n\r", ch );
        return;
    }
    
    if ( (skill = get_skill(ch, gsn_leg_sweep)) ==0 )
    {
        send_to_char("You brush dust off your legs.\n\r", ch);
        return;
    }
        
    ptc(ch, "You sweep your leg in a wide circle!\n\r");
    WAIT_STATE(ch, skill_table[gsn_leg_sweep].beats);
    
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;
        if ( vch != ch 
            && !is_safe_spell(ch,vch,TRUE)
            && !IS_AFFECTED(vch, AFF_ROOTS)
            && (ch_is_flying || !IS_AFFECTED(vch, AFF_FLYING))
            && !(vch->position < POS_FIGHTING))
        {
            check_killer(ch, vch);
            start_combat(ch, vch);
            if ( per_chance(skill) && combat_maneuver_check(ch, vch, gsn_leg_sweep, STAT_AGI, STAT_AGI, 40) )
            {
                act("$n sweeps your legs out from under you!", ch, NULL, vch, TO_VICT);
                act("You leg sweep $N and $N goes down!", ch, NULL, vch, TO_CHAR);
                act("$n leg sweeps $N, sending $M to the ground.", ch, NULL, vch, TO_NOTVICT);
                tally++;
                DAZE_STATE(vch, skill_table[gsn_leg_sweep].beats);
                set_pos(vch, POS_RESTING);
                destance(vch, get_mastery(ch, gsn_leg_sweep));
                int dam = martial_damage(ch, vch, gsn_leg_sweep) * (3 + vch->size) / 10;
                full_dam(ch, vch, dam, gsn_leg_sweep, DAM_BASH, TRUE);
            }
        }
    }
    
    if ( tally == 0 && !per_chance(skill) )
    {
        act("$n falls to the ground in an attempt at a leg sweep.", ch, NULL, NULL, TO_ROOM);
        act("You fall to the ground in an attempt at a leg sweep.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch, gsn_leg_sweep, FALSE, 2);
        set_pos(ch, POS_RESTING);
        check_lose_stance(ch);
    } 
    else  
        check_improve(ch, gsn_leg_sweep, TRUE, 2);
}

DEF_DO_FUN(do_uppercut)
{
    CHAR_DATA *victim;
    int chance, skill;
    
    if ( (skill = get_skill(ch,gsn_uppercut)) == 0 )
    {   
        send_to_char("You are a lover, not a boxer.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    chance = dodge_adjust_chance(ch, victim, skill);
    
    WAIT_STATE( ch, skill_table[gsn_uppercut].beats );
    check_killer(ch,victim);
    
    if ( per_chance(chance) )
    {
        int dam = martial_damage(ch, victim, gsn_uppercut);
        dam = number_range(dam, 3*dam);

        check_improve(ch, gsn_uppercut, TRUE, 3);
        
        if ( !saves_physical(victim, ch, ch->level, DAM_BASH) )
        {
            act("$n stuns you with a crushing right hook!", ch, NULL, victim, TO_VICT);
            act("You stun $N with a crushing right hook!", ch, NULL, victim, TO_CHAR);
            act("$n stuns $N with a crushing right hook.", ch, NULL, victim, TO_NOTVICT);
            DAZE_STATE(victim, skill_table[gsn_uppercut].beats * 3/2);
            WAIT_STATE(victim, skill_table[gsn_uppercut].beats * 3/4);
            destance(victim, get_mastery(ch, gsn_uppercut));
            set_pos( victim, POS_RESTING );
        } 
        full_dam(ch, victim, dam, gsn_uppercut, DAM_BASH, TRUE);
    }
    else
    {
        full_dam(ch, victim, 0, gsn_uppercut, DAM_BASH, TRUE);
        check_improve(ch, gsn_uppercut, FALSE, 3);
    }
}

DEF_DO_FUN(do_war_cry)
{
    AFFECT_DATA af;
    CHAR_DATA *vch;
    int skill, cost, chance, level, modifier;
    int mastery = get_mastery(ch, gsn_war_cry);
    
    if ( (skill = get_skill(ch, gsn_war_cry)) == 0 )
    {
        send_to_char("Your war cry is rather pathetic.\n\r",ch);
        return;
    }
    
    cost = skill_table[gsn_war_cry].min_mana * 200 / (100 + skill);
    level = ch->level * (100 + skill) / 200;
    chance = (100 + skill) / 2;
    modifier = 5 + level/5 + (mastery ? 5 : 0);
    
    if ( ch->move < cost )
    {
        send_to_char("You can't seem to psych yourself up for it.\n\r",ch);
        return;
    }
    
    WAIT_STATE( ch, skill_table[gsn_war_cry].beats );

    if ( !per_chance(chance) )
    {
        ch->move -= cost/2;
        send_to_char("Your war cry isn't very inspirational.\n\r", ch);
        act("$n embarrasses $mself trying to psych up the troops.", ch, NULL, NULL, TO_ROOM);
        check_improve(ch, gsn_war_cry, FALSE, 3);
        return;
    }
        
    ch->move -= cost;
    send_to_char("You scream out a rousing war cry!\n\r",ch);
    act("$n screams a rousing war cry!",ch,NULL,NULL,TO_ROOM);
    check_improve(ch, gsn_war_cry, TRUE, 3);
        
    af.where     = TO_AFFECTS;
    af.type      = gsn_war_cry;
    af.level     = level;
    af.duration  = get_duration(gsn_war_cry, level) * (100 + get_skill_overflow(ch, gsn_war_cry)) / 100;
    af.bitvector = 0;

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( is_same_group(vch, ch) && !is_affected(vch, gsn_war_cry) )
        {
            send_to_char("You feel like killing something.\n\r", vch);
            if ( vch != ch )
                act("Your warcry inspires $N.", ch, NULL, vch, TO_CHAR);
            af.modifier = modifier;
            af.location = APPLY_HITROLL;
            affect_to_char(vch, &af);
            af.location = APPLY_DAMROLL;
            affect_to_char(vch, &af);
            // more mastery boni
            if ( mastery )
            {
                af.modifier = -100;
                af.location = APPLY_AC;
                affect_to_char(vch, &af);
                if ( mastery > 1 )
                {
                    af.modifier = -10;
                    af.location = APPLY_SAVES;
                    affect_to_char(vch, &af);                    
                }
            }
        }
    }
}

DEF_DO_FUN(do_guard)
{
    CHAR_DATA *victim;
    int chance;
    
    if ( (chance = get_skill(ch,gsn_guard)) == 0)
    {
        send_to_char("You don't know how to guard.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( !can_see_combat( ch, victim ) )
    {
        send_to_char( "You can't see them well enough for that.\n\r", ch );
        return;
    }
    
    if (is_safe(ch,victim))
    {
        send_to_char( "You can't guard your opponent in a safe room.\n\r", ch);
        return;
    }
 
    if (ch == victim)
    {
        send_to_char( "You try to guard yourself but just end up looking like a fool.\n\r", ch);
        return;
    }
 
    if ( is_affected(victim, gsn_guard) )
    {
        act("You are already guarding against $N's attacks.",ch,NULL,victim,TO_CHAR);
        return;
    }
    
    /* dex */
    chance += get_curr_stat(ch,STAT_DEX)/8;
    chance -= get_curr_stat(victim,STAT_AGI)/8;
    
    /* now the attack */
    if (number_percent() < chance)
    {
        AFFECT_DATA af;
        
        act("$n vigilantly guards against your attack.",ch,NULL,victim,TO_VICT);
        act("You vigilantly guard against $N's attack.",ch,NULL,victim,TO_CHAR);
        act("$n vigilantly guards against $N's attack.",ch,NULL,victim,TO_NOTVICT);
        check_improve(ch,gsn_guard,TRUE,3);
        WAIT_STATE(ch,skill_table[gsn_guard].beats);
        
        af.where    = TO_AFFECTS;
        af.type     = gsn_guard;
        af.level    = ch->level;
        af.duration = get_duration(gsn_guard, ch->level);
        af.location = APPLY_HITROLL;
        af.modifier = -(ch->level*chance/1000);
        af.bitvector    = AFF_GUARD;
        
        affect_to_char(victim,&af);
    }
    else
    {
        act("You can't keep track of $N.",ch,NULL,victim,TO_CHAR);
        
        WAIT_STATE(ch,skill_table[gsn_guard].beats*2/3);
        check_improve(ch,gsn_guard,FALSE,3);
    } 
    check_killer(ch,victim);
}


DEF_DO_FUN(do_tumble)
{
    int chance;
    char arg[MAX_INPUT_LENGTH];
    int cost = 40;
    
    if ((chance = get_skill(ch,gsn_tumbling)) == 0)
    {
        send_to_char("You aren't an acrobat.\n\r",ch);
        return;
    }
    
    one_argument(argument,arg);
    
    if ((arg[0] == 's') || (arg[0] == 'S'))
    {
        send_to_char("You stop tumbling.\n\r", ch);
		affect_strip(ch, gsn_tumbling);
		return;
    }
    
    if (is_affected(ch,gsn_tumbling))
    {
        send_to_char("You are already tumbling around.\n\r",ch);
        return;
    }
    
    if (IS_AFFECTED(ch,AFF_CALM))
    {
        send_to_char("You're feeling too mellow to tumble.\n\r",ch);
        return;
    }
    
    if (ch->move < cost)
    {
        send_to_char("You can't get up enough energy.\n\r",ch);
        return;
    }
    
    /* modifiers */
    
    /* fighting */
    if (ch->position == POS_FIGHTING)
        chance -= 10;
    
    chance += get_curr_stat(ch,STAT_AGI)/4 - 10;
    
    WAIT_STATE(ch,skill_table[gsn_tumbling].beats);
    
    if (number_percent() < chance)
    {
        AFFECT_DATA af;
        
        ch->move -= cost;
        
        send_to_char("You begin to tumble around avoiding attacks.\n\r",ch);
        act("$n tumbles around avoiding attacks.",ch,NULL,NULL,TO_ROOM);
        check_improve(ch,gsn_tumbling,TRUE,3);
        
        af.where    = TO_AFFECTS;
        af.type     = gsn_tumbling;
        af.level    = ch->level;
        af.duration = get_duration(gsn_tumbling, ch->level);
     /* Changed from level divided by 10, to 25. Makes
        Tumble actually worth using. -Astark Nov 2012 */
        af.modifier = UMIN(-1,-(ch->level/25));
        af.bitvector    = 0;
        
        af.location = APPLY_HITROLL;
        affect_to_char(ch,&af);
        
        af.location = APPLY_DAMROLL;
        affect_to_char(ch,&af);
        
        af.modifier = UMIN(-10,-10 * (ch->level/5));
        af.location = APPLY_AC;
        affect_to_char(ch,&af);
    }
    
    else
    {
        ch->move -= cost/2;
        
        act("$n tumbles to the ground painfully.",ch,NULL,NULL,TO_ROOM);
        send_to_char("You tumble to the ground painfully.\n\r",ch);
        check_improve(ch,gsn_tumbling,FALSE,3);
        DAZE_STATE(ch, PULSE_VIOLENCE);

        check_lose_stance(ch);

        set_pos( ch, POS_RESTING );
        damage(ch,ch,number_range(4, 6 +  3 * ch->size),gsn_tumbling,
            DAM_BASH,TRUE);
    }
}




DEF_DO_FUN(do_feint)
{
    CHAR_DATA *victim = NULL;
    CHAR_DATA *fch = NULL;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int chance;
    
    if ((chance = get_skill(ch,gsn_feint)) == 0)
    {
        send_to_char("You don't know how.\n\r",ch);
        return;
    }
    
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if ( vch->fighting == ch )
            fch = vch;
    }
    
    if ( fch == NULL )
    {
        send_to_char( "Nobody is attacking you.\n\r", ch );
        return;
    }
    
    /*
    if (in_pkill_battle (ch))
    {
	send_to_char( "You cannot feint from a pk battle.\n\r", ch );
	return;
    }
    */
    
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if ( is_same_group(ch, vch) && ch != vch && vch->fighting )
            victim = vch;
    }
    
    if (victim == NULL)
    {
        send_to_char("You can't find anyone to take the fall.\n\r", ch);
        return;
    }
    
    WAIT_STATE( ch, skill_table[gsn_feint].beats );
    if ( number_percent( ) > chance)
    {
        send_to_char( "You fail to get away.\n\r", ch );
        check_improve(ch,gsn_feint,FALSE,2);
        return;
    }
    
    act( "You feint away from $N!",  ch, NULL, fch, TO_CHAR    );
    act( "$n feints away from you!", ch, NULL, fch, TO_VICT    );
    act( "$n feints away from $N!",  ch, NULL, fch, TO_NOTVICT );
    check_improve(ch,gsn_feint,TRUE,2);
    
    set_fighting(fch, victim);
    return;
}





DEF_DO_FUN(do_distract)
{
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int chance;
    
    if ((chance = get_skill(ch,gsn_distract)) == 0)
    {
        send_to_char("You don't know how.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( victim == ch )
    {
        send_to_char( "You attempt to distract yourself.\n\r", ch );
        return;
    }
    
    if ( victim->fighting == NULL )
    {
        send_to_char( "That person is not fighting right now.\n\r", ch );
        return;
    }

    if( is_safe(ch,victim) )
        return;
    
    chance /= 2;
    chance += ch->level - victim->level;
    chance -= get_curr_stat(victim,STAT_WIS)/8;
    chance += get_curr_stat(victim,STAT_INT)/8;
    if (chance > 70)
	chance = chance/2+35;
    else if (chance < 30)
	chance = chance/2 + 15;
    
    WAIT_STATE( ch, skill_table[gsn_distract].beats );
    if ( number_percent( ) > chance)
    {
        send_to_char( "You fail to create a distraction.\n\r", ch );
        check_improve(ch,gsn_distract,FALSE,3);
        return;
    }
    
    act( "You distract $N!",  ch, NULL, victim, TO_CHAR    );
    act( "$n distracts you!", ch, NULL, victim, TO_VICT    );
    act( "$n distracts $N!",  ch, NULL, victim, TO_NOTVICT );
    check_improve(ch,gsn_distract,TRUE,3);
    
    check_lose_stance(victim);

    stop_fighting( victim, FALSE );
    if (IS_NPC(victim))
    {
	stop_hunting(victim);
	SET_BIT(victim->off_flags, OFF_DISTRACT);
    }
    
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if (vch->fighting == victim)
            stop_fighting(vch, FALSE);
    }
    
    return;
}

DEF_DO_FUN(do_chop)
{
    CHAR_DATA *victim;
    int dam, chance;
    
    if (get_skill(ch,gsn_chop)==0)
    {
        send_to_char(
            "You better leave the martial arts to fighters.\n\r", ch );
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
        
        chance = get_skill(ch, gsn_chop);
        
        check_killer(ch,victim);
        mastery_adjusted_wait(ch, gsn_chop);

        if ( check_hit(ch, victim, gsn_chop, DAM_SLASH, chance) )
        {
            dam = martial_damage( ch, victim, gsn_chop );
            full_dam(ch,victim, dam, gsn_chop,DAM_SLASH,TRUE);
            check_improve(ch,gsn_chop,TRUE,3);
        }
        else
        {
            damage( ch, victim, 0, gsn_chop,DAM_SLASH,TRUE);
            check_improve(ch,gsn_chop,FALSE,3);
        }
        return;
}

DEF_DO_FUN(do_bite)
{
    CHAR_DATA *victim;
    int dam, chance;
    
    chance=get_skill(ch, gsn_bite);
    chance=UMAX(chance,get_skill(ch, gsn_venom_bite));
    chance=UMAX(chance,get_skill(ch, gsn_vampiric_bite));
    
    if ( IS_SET(ch->parts, PART_FANGS) )
	chance = (chance + 100) / 2;

    if (chance==0)
    {
        send_to_char("Your teeth aren't that strong.\n\r", ch );
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
        
        mastery_adjusted_wait(ch, gsn_bite);
        check_killer(ch,victim);

        if ( check_hit(ch, victim, gsn_bite, DAM_PIERCE, chance) )
        {
            dam = martial_damage( ch, victim, gsn_bite );
            full_dam(ch,victim, dam, gsn_bite,DAM_PIERCE,TRUE);
            check_improve(ch,gsn_bite,TRUE,3);
	    CHECK_RETURN(ch, victim);
	    
            if (get_skill(ch,gsn_venom_bite)>number_percent())
	    {
                poison_effect(victim, ch->level,dam,TARGET_CHAR);
	    }
            else if (get_skill(ch,gsn_vampiric_bite)>number_percent())
            {
                /*dam = 1 + ch->level/2;*/
                act("$n drains the life from $N.",ch,NULL,victim,TO_NOTVICT);
                act("You feel $N drawing your life away.",victim,NULL,ch,TO_CHAR);
                /*damage(ch,victim,dam,0,DAM_NEGATIVE,FALSE);*/
                if (number_bits(4) == 0)
		    drop_align( ch );
                gain_hit(ch, dam/5);
            }
        }
        else
        {
            damage( ch, victim, 0, gsn_bite,DAM_PIERCE,TRUE);
            check_improve(ch,gsn_bite,FALSE,3);
        }
        return;
}


void behead(CHAR_DATA *ch, CHAR_DATA *victim)
{
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
    const char *name;
	act( "$n's severed head plops on the ground.", victim, NULL, NULL, TO_ROOM );
	damage(ch,victim, 0, gsn_beheading,0,TRUE);
        
    CHAR_DATA *leader = get_local_leader(ch);
    if ( !IS_NPC(leader) )
    {
        leader->pcdata->behead_cnt += 1;
        update_lboard(LBOARD_BHD, leader, leader->pcdata->behead_cnt, 1);
    }
    
	name = IS_NPC(victim) ? victim->short_descr : victim->name;
	obj  = create_object_vnum(OBJ_VNUM_SEVERED_HEAD);
	obj->timer  = -1;
					 
	sprintf( buf, obj->short_descr, name );
	free_string( obj->short_descr );
	obj->short_descr = str_dup( buf );
					 
	sprintf( buf, obj->description, name );
	free_string( obj->description );
	obj->description = str_dup( buf );
					 
	obj_to_room( obj, ch->in_room );
}

DEF_DO_FUN(do_double_strike)
{
    CHAR_DATA *victim;
    int skill;

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
	return;

    if ( get_eq_char(ch, WEAR_SECONDARY) == NULL )
    {
	send_to_char( "You need to wield two weapons to double strike.\n\r", ch );
	return;
    }

    if ( (skill = get_skill(ch, gsn_double_strike)) == 0 )
    {
	send_to_char( "You don't know how to double strike.\n\r", ch );
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_double_strike].beats );

    if ( !per_chance(skill) )
    {
	send_to_char( "You stumble and fail.\n\r", ch );
	check_improve( ch, gsn_double_strike, FALSE, 3 );
    }
    else
    {
        int hits = 0;

        act( "You strike out at $N!", ch, NULL, victim, TO_CHAR );
        if ( one_hit(ch, victim, gsn_double_strike, FALSE) )
            hits++;
        if ( one_hit(ch, victim, gsn_double_strike, TRUE) )
            hits++;
        check_improve( ch, gsn_double_strike, TRUE, 3 );

        if ( stop_attack(ch, victim) )
            return;

        int rend_chance = mastery_bonus(ch, gsn_double_strike, 40, 50) + get_skill_overflow(ch, gsn_double_strike) * 2/5;
        if ( hits == 2 && per_chance(rend_chance) )
        {
            act("You bury your weapons deep in $N, then rip them out sideways!", ch, NULL, victim, TO_CHAR);
            act("$n buries $s weapons deep in your body, then rips them out sideways!", ch, NULL, victim, TO_VICT);
            act("$n buries $s weapons deep in $N, then rips them out sideways!", ch, NULL, victim, TO_NOTVICT);
            int dam = one_hit_damage(ch, victim, gsn_double_strike, get_eq_char(ch, WEAR_WIELD))
                    + one_hit_damage(ch, victim, gsn_double_strike, get_eq_char(ch, WEAR_SECONDARY));
            int dt_rend = TYPE_HIT + 102; // see attack_table in const.c
            deal_damage(ch, victim, dam, dt_rend, DAM_SLASH, TRUE, TRUE);
        }
    }
}

DEF_DO_FUN(do_round_swing)
{
    CHAR_DATA *vch, *vch_next;
    OBJ_DATA *wield;
    int skill;

    wield = get_eq_char(ch, WEAR_WIELD);
    
    if ( is_ranged_weapon(wield) )
    {
        ptc(ch, "That's not how you use a ranged weapon!\n\r");
        return;
    }
    
    if ( wield == NULL || !IS_WEAPON_STAT(wield, WEAPON_TWO_HANDS) )
    {
	send_to_char( "You need to wield a two-handed weapon.\n\r", ch );
	return;
    }

    if ( (skill = get_skill(ch, gsn_round_swing)) == 0 )
    {
	send_to_char( "You don't know how to perform a round swing.\n\r", ch );
	return;
    }

    int swing_time = skill_table[gsn_round_swing].beats;
    int mastery = mastery_bonus(ch, gsn_round_swing, 20, 25);
    swing_time = rand_div(swing_time * (100 - mastery), 100);
    WAIT_STATE( ch, swing_time );

    if ( per_chance(50) && !per_chance(skill) )
    {
        send_to_char("You stumble and fall to the ground.\n\r", ch);
        act("$n tries to swing $s weapon but stumbles.", ch, NULL, NULL, TO_ROOM);
        set_pos(ch, POS_RESTING);
        check_improve(ch, gsn_round_swing, FALSE, 2);
        return;
    }

    send_to_char( "You spin around fiercely!\n\r", ch );
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
	vch_next = vch->next_in_room;
	if ( vch != ch && !is_safe_spell(ch,vch,TRUE))
	    one_hit( ch, vch, gsn_round_swing, FALSE );
    }
    check_improve( ch, gsn_round_swing, TRUE, 2 );
}

DEF_DO_FUN(do_spit)
{
    CHAR_DATA *victim;
    int skill;
    
    if ( (skill = get_skill(ch,gsn_spit)) == 0 )
    {
        send_to_char("You spit in utter disgust!!\n\r", ch);
        act("$n spits in utter disgust!!", ch, NULL, NULL, TO_ROOM);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if (IS_AFFECTED(victim,AFF_BLIND))
    {
        act("$E's already been blinded.",ch,NULL,victim,TO_CHAR);
        return;
    }
    
    if (victim == ch)
    {
        send_to_char("Very funny.\n\r",ch);
        return;
    }
    
    check_killer(ch,victim);
    WAIT_STATE(ch, skill_table[gsn_spit].beats);
    start_combat(ch, victim);
        
    /* now the attack */
    if ( per_chance(skill) && combat_maneuver_check(ch, victim, gsn_spit, STAT_DEX, STAT_AGI, 50) )
    {
        AFFECT_DATA af;
        act("$n is blinded by the glob of spit in $s eyes!",victim,NULL,NULL,TO_ROOM);
        act("$n places a glob of spit in your eyes!",ch,NULL,victim,TO_VICT);
        send_to_char("You can't see a thing!\n\r",victim);

        check_improve(ch,gsn_spit,TRUE,3);
        
        af.where    = TO_AFFECTS;
        af.type     = gsn_spit;
        af.level    = ch->level;
        af.duration = 0;
        af.location = APPLY_HITROLL;
        af.modifier = -4;
        af.bitvector    = AFF_BLIND;
        
        affect_to_char(victim,&af);
    }
    else
    {
        damage(ch,victim,0,gsn_spit,DAM_DROWNING,TRUE);
        check_improve(ch,gsn_spit,FALSE,3);
    }
}

DEF_DO_FUN(do_choke_hold)
{
    CHAR_DATA *victim;
    int dam, skill;
    
    if ( (skill = get_skill(ch,gsn_choke_hold)) == 0)
    {
        send_to_char("You don't know how to apply a choke hold.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if (ch == victim)
    {
        send_to_char( "You try to choke yourself but just end up looking like a fool.\n\r", ch);
        return;
    }
 
    if ( is_affected( victim, gsn_choke_hold ))
    {
        act("$N is already choking.",ch,NULL,victim,TO_CHAR);
        return;
    }
    
    WAIT_STATE(ch,skill_table[gsn_choke_hold].beats);
    check_killer(ch,victim);
    start_combat(ch, victim);
    
    /* now the attack */
    if ( per_chance(skill) && combat_maneuver_check(ch, victim, gsn_choke_hold, STAT_DEX, STAT_AGI, 50) )
    {
        AFFECT_DATA af;
        
        act("$n grabs you by the neck and begins to squeeze.",ch,NULL,victim,TO_VICT);
        act("You grab $N by the neck and begin to squeeze.",ch,NULL,victim,TO_CHAR);
        act("$n grabs $N by the neck and begins to squeeze.",ch,NULL,victim,TO_NOTVICT);
        
        check_improve(ch,gsn_choke_hold,TRUE,3);
        
        af.where    = TO_AFFECTS;
        af.type     = gsn_choke_hold;
        af.level    = get_curr_stat(ch, STAT_STR);
        af.duration = -1; // removed in special_affect_update
        af.location = APPLY_HITROLL;
        af.modifier = -ch->level / 5;
        af.bitvector = AFF_GUARD;
        affect_to_char(victim,&af);
        
        dam = martial_damage(ch, victim, gsn_choke_hold) / 2;
        full_dam(ch, victim, dam, gsn_choke_hold, DAM_BASH, TRUE);
    }
    else
    {
        act("You try to wring $N's neck but fail.",ch,NULL,victim,TO_CHAR);
        check_improve(ch,gsn_choke_hold,FALSE,3);
    }
}

DEF_DO_FUN(do_roundhouse)
{
   int tally=0;
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   int skill, dam; 

   dam = martial_damage( ch, NULL, gsn_roundhouse );

   if ( (skill = get_skill(ch, gsn_roundhouse)) == 0 )
   {
       send_to_char( "You wouldn't know how to do that.\n\r", ch );
       return;
   }

   ptc(ch, "You swing your leg in a wild kick!\n\r");
   
   for ( vch = ch->in_room->people; vch != NULL; vch = vch_next)
   {
       vch_next = vch->next_in_room;
       if ( vch != ch && !is_safe_spell(ch,vch,TRUE))
       {
	   /* now the attack */
	   if ( check_hit(ch, vch, gsn_roundhouse, DAM_BASH, skill) )
	   {
	       check_killer(ch,vch);
	       tally++;
	       full_dam(ch,vch,dam,gsn_roundhouse, DAM_BASH, TRUE);
	   }
        else
           full_dam(ch, vch, 0, gsn_roundhouse, DAM_BASH, TRUE);
       }
   }

   if ( tally==0 && !per_chance(skill) )
   {
      act("$n falls to the ground in an attempt at a roundhouse kick.",ch,NULL,NULL,TO_ROOM);
      act("You fall to the ground in an attempt at a roundhouse kick.",ch,NULL,NULL,TO_CHAR);
      check_improve(ch, gsn_roundhouse, FALSE,2);
      check_lose_stance(ch);
      DAZE_STATE(ch, 2*PULSE_VIOLENCE);
      set_pos( ch, POS_RESTING );
      damage(ch,ch,number_range(4, 6 +  3 * ch->size),gsn_roundhouse, DAM_BASH,TRUE);
   }
   else
      check_improve(ch, gsn_roundhouse, TRUE, 2);

   WAIT_STATE( ch, skill_table[gsn_roundhouse].beats );

   return;
}

DEF_DO_FUN(do_hurl)
{
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int skill, dam;
    
    if ((skill = get_skill(ch,gsn_hurl)) == 0)
    {
        send_to_char("You feel the bile rising from your guilty past.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( victim == ch )
    {
        send_to_char( "You grab yourself by the arm and hurl yourself to the floor.\n\r", ch );
        return;
    }
    
    if ( victim->fighting  == NULL )
    {
        send_to_char( "That person is not fighting right now.\n\r", ch );
        return;
    }

    if ( IS_AFFECTED(victim, AFF_ROOTS) )
    {
        act("$N is rooted firmly to the ground.", ch, NULL, victim, TO_CHAR);
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_hurl].beats );
        
    if ( per_chance(skill) && combat_maneuver_check(ch, victim, gsn_hurl, STAT_AGI, STAT_STR, 33) )
    {
        act("You hurl $N across the room!", ch, NULL, victim, TO_CHAR);
        act("$n hurls you!", ch, NULL, victim, TO_VICT);
        act("$n hurls $N across the room!", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch,gsn_hurl,TRUE,3);
        
        dam = martial_damage(ch, victim, gsn_hurl) * (3 + victim->size) / 5;
        
        DAZE_STATE( victim, 2*skill_table[gsn_hurl].beats );
        WAIT_STATE( victim, skill_table[gsn_hurl].beats );
        full_dam(ch, victim, dam, gsn_hurl, DAM_BASH, TRUE);
        
        destance(victim, get_mastery(ch, gsn_hurl));
        set_pos(victim, POS_RESTING);
        stop_fighting(victim, FALSE);
        
        for ( vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (vch->fighting == victim)
                stop_fighting(vch, FALSE);
        }
    }
    else
    {
        send_to_char( "You fail to hurl your opponent.\n\r", ch );
        check_improve(ch,gsn_hurl,FALSE,3);
        return;
    }
    
    return;
}

/* Mug attempted by Mephiston (his words), fixed by Siva 4/15/98*/
DEF_DO_FUN(do_mug)
{
    char buf  [MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int skill, chance;
    int dam;
    int gold, silver;
    
    if (IS_NPC(ch))
        return;
    
    skill = get_skill(ch,gsn_mug);
    
    if (skill < 1)
    {    
        send_to_char("You are the one who would probably get mugged.\n\r",ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;

    if ( victim == ch )
    {
        send_to_char( "You wish.\n\r", ch );
        return;
    }
    
    if (is_safe(ch,victim))
        return;

    chance = dodge_adjust_chance(ch, victim, skill);
    
    check_killer(ch,victim);
    WAIT_STATE( ch, skill_table[gsn_mug].beats );
    
    /* Successful roll */
    
    if ( per_chance(chance) )
    {
        dam = martial_damage(ch, victim, gsn_mug);
        
        full_dam(ch, victim, dam, gsn_mug, DAM_BASH, TRUE);
        check_improve(ch,gsn_mug,TRUE,3);
        
	/* no stealing in warfare --Bobble */
	if ( IS_SET(ch->act, PLR_WAR) )
	    return;

        if ( combat_maneuver_check(ch, victim, gsn_mug, STAT_STR, STAT_STR, 50) )
        {
            gold   = victim->gold   * number_range(1, 100) / 500;
            silver = victim->silver * number_range(1, 100) / 500;
            
            // bonus money created out of thin air
            long bonus = 5 * level_base_wealth(victim->level) * UMIN(dam, victim->max_hit) / UMAX(1, victim->max_hit);
            int gold_gain = gold + bonus / 100;
            int silver_gain = silver + bonus % 100;
            
            if ( gold_gain <= 0 && silver_gain <= 0 )
            {
                send_to_char( "You rifle through their purse, but you don't feel any coins.\n\r", ch );
                return;
            }
            
            ch->gold   += gold_gain;
            ch->silver += silver_gain;
            victim->silver -= silver;
            victim->gold   -= gold;
            
            if ( silver_gain <= 0 )
                ptc(ch, "Bingo!  You got %d gold coins.\n\r", gold_gain);
            else if ( gold_gain <= 0 )
                ptc(ch, "Bingo!  You got %d silver coins.\n\r", silver_gain);
            else
                ptc(ch, "Bingo!  You got %d silver and %d gold coins.\n\r", silver_gain, gold_gain);
            
            if ( IS_IMMORTAL(ch) )
                ptc(ch, "Mugging bonus is %d silver and %d gold.\n\r", bonus % 100, bonus / 100);
                
            if( !IS_NPC(ch) && !IS_NPC(victim) )
                adjust_pkgrade( ch, victim, TRUE ); /* True means it's a theft */
            
            check_improve(ch,gsn_mug,TRUE,3);
            
            if ( per_chance(10) )
            {
                OBJ_DATA *obj_check;
                OBJ_DATA *obj_found = NULL;
                OBJ_DATA *obj_next;
                
                for ( obj_check = victim->carrying; obj_check != NULL; obj_check = obj_next )
                {
                    obj_next = obj_check->next_content;
                    if ( obj_check->wear_loc == -1
                        && can_drop_obj(victim,obj_check)
                        && (obj_check->level < ch->level-2) )
                    {
                        obj_found = obj_check;
                        break;
                    }
                }
                
                if ( obj_found != NULL && can_steal(ch, victim, obj_found, FALSE) )
                {
                    obj_from_char( obj_found );
                    obj_to_char( obj_found, ch );
                    act("Bingo! You snag $p from $N.",ch, obj_found, victim, TO_CHAR);
                    if ( !IS_NPC(victim) )
                    {
                        sprintf( buf, "$N stole $p from %s.", victim->name );
                        wiznet( buf, ch, obj_found, WIZ_FLAGS, 0, 0 );

                        if( !IS_NPC(ch) )
                            adjust_pkgrade( ch, victim, TRUE ); /* True means it's a theft */
                    }
                    check_improve(ch,gsn_mug,TRUE,3);
                    return;
                } /* end obj_found != NULL */
            }  /* end check for chance of stealing items */
        }  /* end check for chance of stealing coins */
    }
    else
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }
}


DEF_DO_FUN(do_fatal_blow)
{
    CHAR_DATA *victim;
    int skill, chance;
    
    if ( (skill = get_skill(ch,gsn_fatal_blow)) < 1 )
    {
        send_to_char( "Better not, you might hurt yourself.\n\r", ch );
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
        
    chance = dodge_adjust_chance(ch, victim, skill);
        
    check_killer(ch,victim);
    WAIT_STATE( ch, skill_table[gsn_fatal_blow].beats );
        
    if ( per_chance(chance) )
    {
        // initial punch to set up
        int dam = martial_damage(ch, victim, gsn_fatal_blow);
        full_dam(ch, victim, dam, gsn_fatal_blow, DAM_BASH, TRUE);
        CHECK_RETURN(ch, victim);
        // second blow for massive damage
        int move_loss = IS_AFFECTED(ch, AFF_BERSERK) ? ch->move / 8 : ch->move / 12;
        move_loss += move_loss * mastery_bonus(ch, gsn_fatal_blow, 15, 25) / 100;
        ch->move -= move_loss;
        dam += move_loss * 4;
            
        /* chance to stun */
        int stun_level = (ch->level/2 + move_loss/4);
        if ( number_bits(1) && !saves_physical(victim, ch, stun_level, DAM_BASH) )
        {
            act( "You stun $N with a crushing blow to $S temple!", ch, NULL, victim, TO_CHAR );
            act( "$n stuns you with a crushing blow to your temple!", ch, NULL, victim, TO_VICT );
            act( "$n stuns $N with a crushing blow to $S temple!", ch, NULL, victim, TO_NOTVICT );
            destance(victim, get_mastery(ch, gsn_fatal_blow));
            WAIT_STATE( victim, skill_table[gsn_fatal_blow].beats * 3/4 );
            DAZE_STATE( victim, skill_table[gsn_fatal_blow].beats * 3/2 );
        }
        full_dam(ch, victim, dam, gsn_fatal_blow, DAM_BASH, TRUE);
        check_improve(ch, gsn_fatal_blow, TRUE, 3);
    }
    else
    {
        damage(ch, victim, 0, gsn_fatal_blow, DAM_BASH,TRUE);
        check_improve(ch,gsn_fatal_blow, FALSE,3);
    }
    return;
}

DEF_DO_FUN(do_intimidate)
{
    CHAR_DATA *victim;
    int skill;
    int level;

    if ( (skill = get_skill(ch, gsn_intimidation)) == 0 )
    {
	send_to_char( "You have no clue how to do that.\n\r", ch );
	return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
	return;

    if ( victim == ch )
    {
	send_to_char( "You look sooo scary.\n\r", ch );
	return;
    }

    if ( !IS_AFFECTED(victim, AFF_SANCTUARY) ) 
    {
        send_to_char( "Your opponent isn't protected by Sanctuary.\n\r",ch);
        return;
    }
    
    if ( !can_see_combat(victim, ch) )
	skill /= 2;
    
    /* bonus if victim is already scared */
    if ( IS_AFFECTED(victim, AFF_FEAR) )
	level = ch->level + 20;
    else
	level = ch->level;

    WAIT_STATE( ch, skill_table[gsn_intimidation].beats );
    if ( !chance(skill) || IS_AFFECTED(victim, AFF_HEROISM) || saves_spell(victim, ch, level, DAM_MENTAL) )
    {
        act( "You don't really intimidate $N.", ch, NULL, victim, TO_CHAR );
        act( "$n tried to intimidate you, but you won't take $s crap.",
	     ch, NULL, victim, TO_VICT );
	check_improve( ch, gsn_intimidation, FALSE, 2 );
	full_dam( ch, victim, 0, gsn_intimidation, DAM_MENTAL, FALSE );
        return;
    }
    check_improve( ch, gsn_intimidation, TRUE, 2 );
    
    if ( check_dispel(level, victim, skill_lookup("sanctuary")) )
    {
        act( "You intimidate $N out of $S comfy sanctuary.",
	     ch, NULL, victim, TO_CHAR);
        act( "$n intimidates you out of your comfy sanctuary.",
	     ch, NULL, victim, TO_VICT);
        act( "$n intimidates $N out of $S comfy sanctuary.",
	     ch, NULL, victim, TO_NOTVICT);
    }
    else
        send_to_char("Nope, that didn't quite work.\n\r",ch);
    full_dam( ch, victim, ch->level, gsn_intimidation, DAM_MENTAL, TRUE );
}

DEF_DO_FUN(do_crush)
{
    CHAR_DATA *victim;
    int dam, power;

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
	return;
    
    if ( ch->size <= victim->size )
    {
	/*
	send_to_char( "You can only crush smaller people.\n\r", ch );
	*/
	return;
    }

    act( "$n crushes down on you with all $s weight.", ch, NULL, victim, TO_VICT );
    act( "$n crushes down on $N with all $s weight.", ch, NULL, victim, TO_NOTVICT );

    if ( check_dodge(ch, victim) )
	return;

    power = ch->size - victim->size;
    dam = power * dice( ch->level, 2 );
    full_dam( ch, victim, dam, gsn_crush, DAM_BASH, TRUE );
    DAZE_STATE( victim, PULSE_VIOLENCE * power );
}


DEF_DO_FUN(do_blackjack)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    int chance, dam, chance_stun;

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( victim == ch )
    {
        send_to_char( "Better not, that would hurt!\n\r", ch );
        return;
    }
    
    if ( is_safe( ch, victim ) )
        return;

    if ((chance = get_skill(ch, gsn_blackjack)) < 1)
    {
        send_to_char("You're not trained at this.\n\r",ch);
        return;
    }

    /*
    if ( !IS_NPC(ch) && get_weapon_sn(ch) != gsn_mace)
    {
        send_to_char( "You need a mace to blackjack.\n\r", ch);
        return;
    }
    */

    if ( IS_AFFECTED(victim, AFF_SLEEP) )
    {
        send_to_char( "Your victim is already knocked out.\n\r", ch);
        return;
    }
    
    if ( check_see_combat(victim, ch) )
    {
	chance += (get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_AGI)) / 8;
	chance -= 75;
    }
    
    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_blackjack].beats );

  if ( ch->position != POS_FIGHTING )
  {    
    if ( number_percent() > chance && IS_AWAKE(victim) )
    {
	act( "You failed to sneak up on $N.", ch, NULL, victim, TO_CHAR );
        damage( ch, victim, 0, gsn_blackjack, DAM_NONE, TRUE);
        check_improve(ch,gsn_blackjack,FALSE,2);
	return;
    }

    if ( saves_physical(victim, ch, ch->level * chance/100, DAM_BASH)
	 || (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_KILL))
	 || IS_UNDEAD(victim) )
    {
	act( "$N staggers but doesn't fall.", ch, NULL, victim, TO_CHAR );
        damage( ch, victim, ch->level, gsn_blackjack, DAM_BASH, TRUE);
        check_improve(ch,gsn_blackjack,FALSE,2);
	return;
    }

    act( "You knock $N out.", ch, NULL, victim, TO_CHAR );
    act( "$n knocks $N out.", ch, NULL, victim, TO_NOTVICT );

    if ( IS_AWAKE(victim) )
    {
	act( "Something hits you on the head and the world turns black.",
	     ch, NULL, victim, TO_VICT );
	stop_fighting(victim, TRUE);
	set_pos( victim, POS_SLEEPING );
    }

    if ( victim->pcdata != NULL )
	victim->pcdata->pkill_timer = 
	    UMAX(victim->pcdata->pkill_timer, 10 * PULSE_VIOLENCE);

    af.where     = TO_AFFECTS;
    af.type      = gsn_blackjack;
    af.level     = ch->level;
    af.duration  = 1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SLEEP;
    affect_join( victim, &af );
  }
  else
  {
     chance += get_skill(ch,gsn_blackjack)*2/3;

     dam = number_range(3,5) * ch->level;
 
   /* Does the blackjack hit ? */
     if ( number_percent() < chance )
     {
         chance_stun = chance/7;
         chance_stun += (get_curr_stat(ch, STAT_STR) - get_curr_stat(victim, STAT_STR)) / 4;
  
        /* check if the attack stuns the opponent */
        if ( !IS_AFFECTED(victim, AFF_ROOTS) && number_percent() < chance_stun )
        {
            act("$n smashes you in the side of the head, stunning you!",ch,NULL,victim,TO_VICT);
            act("You smash $N in the side of $S head, stunning $M!",ch,NULL,victim,TO_CHAR);
            act("$n smashes $N in the side of $S head, stunning $M!",ch,NULL,victim,TO_NOTVICT);
            DAZE_STATE(victim, 2*PULSE_VIOLENCE + ch->size - victim->size );
        }

        damage( ch, victim,dam,gsn_blackjack, DAM_BASH, TRUE);
        check_improve(ch,gsn_blackjack,TRUE,3);
        return;
    }
    else
    {
        damage( ch, victim,0, gsn_blackjack, DAM_BASH, TRUE);
        check_improve(ch,gsn_blackjack,FALSE,3);
        return;
    }         
  }
}


DEF_DO_FUN(do_rake)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    if ( get_skill(ch, gsn_rake) == 0 && !IS_SET(ch->parts, PART_CLAWS) )
    {
       send_to_char( "You lack the claws for that!\n\r", ch );
       return;
    }
        
    WAIT_STATE( ch, skill_table[gsn_rake].beats );
    
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if ( vch != ch && !is_safe_spell(ch, vch, TRUE) )
        {
            check_killer(ch, vch);
            rake_char(ch, vch);
       }
    }
}

// make one rake attack
void rake_char( CHAR_DATA *ch, CHAR_DATA *victim)
{
    int skill = get_skill(ch, gsn_rake);
    
    if ( IS_SET(ch->parts, PART_CLAWS) )
    {
        skill = (100 + skill) / 2;
    }
    
    if ( check_hit(ch, victim, gsn_rake, DAM_SLASH, skill) )
    {
        int razor_skill = get_skill(ch, gsn_razor_claws);
        if ( per_chance(razor_skill) && number_bits(8) == 69 )
        {
            /* behead */
            act("In a mighty strike, your claws separate $N's neck.", ch, NULL, victim, TO_CHAR);
            act("In a mighty strike, $n's claws separate $N's neck.", ch, NULL, victim, TO_NOTVICT);
            act("$n slashes $s claws through your neck.", ch, NULL, victim, TO_VICT);
            behead(ch, victim);
        }
        else
        {
            int dam = martial_damage(ch, victim, gsn_rake);
            full_dam(ch, victim, dam, gsn_rake, DAM_SLASH, TRUE);
        }
    }
    else
        full_dam(ch, victim, 0, gsn_rake, DAM_SLASH, TRUE);
}

// make one mummy slam attack
void mummy_slam( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int skill = get_skill(ch, gsn_mummy_slam);

    if ( check_hit(ch, victim, gsn_mummy_slam, DAM_BASH, skill) )
    {
        int dam = martial_damage(ch, victim, gsn_mummy_slam);
        full_dam(ch, victim, dam, gsn_mummy_slam, DAM_BASH, TRUE);
        
        if ( stop_attack(ch, victim) )
            return;
        
        // mummy rot, i.e., decompose
        int level = ch->level;
        if ( !saves_spell(victim, ch, level, DAM_HARM) )
        {
            if ( is_affected(victim, gsn_decompose) )
            {
                decompose_update(victim, level);
            }
            else
            {
                act("You contract mummy rot!", victim, NULL, NULL, TO_CHAR);
                act("$n is infected with mummy rot!", victim, NULL, NULL, TO_ROOM);
                start_decompose(victim, level);
            }
        }
    }
    else
        full_dam(ch, victim, 0, gsn_mummy_slam, DAM_BASH, TRUE);
}

DEF_DO_FUN(do_puncture)
{
    AFFECT_DATA af;
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    int skill, dam;
    
    if ( (wield = get_eq_char(ch, WEAR_WIELD)) == NULL )
    {
        send_to_char( "With your bare hands?\n\r",ch);
        return;
    }

    if ( (skill = get_skill(ch,gsn_puncture)) == 0)
    {
        send_to_char( "You don't know how to puncture an armor.\n\r",ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
	return;

    if (victim == ch)
    {
        send_to_char( "Sounds like a bad idea.\n\r", ch );
        return;
    }
    
    if ( is_safe(ch,victim) )
        return;
    
    check_killer(ch,victim);
    WAIT_STATE(ch, skill_table[gsn_puncture].beats);

    /* now the attack */
    int chance = dodge_adjust_chance(ch, victim, skill);
    if ( !per_chance(chance) )
    {
        damage(ch,victim,0,gsn_puncture,DAM_NONE,TRUE);
        check_improve(ch,gsn_puncture,FALSE,3);
        return;
    }

    /* hit - how much dam? */
    int fixed = (5 + ch->level) * 2/3;
    int variable = (5 + ch->level) / 3;
    // twohanders deal 50% more base damage
    if ( IS_WEAPON_STAT(wield, WEAPON_TWO_HANDS) )
        fixed = 5 + ch->level;
    dam = fixed + number_range(0, variable);
    // dual-wielding adds variable damage twice
    if ( get_eq_char(ch, WEAR_SECONDARY) != NULL )
        dam += number_range(0, variable);
    // damroll helps as well
    dam += get_damroll(ch) / 12;
    // mastery bonus
    dam += dam * mastery_bonus(ch, gsn_puncture, 20, 25) / 100;
    // sanc reduces damage by 25%
    if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
        dam -= dam / 4;

    act( "You puncture $N's armor with a powerful blow!",
	 ch, NULL, victim, TO_CHAR );
    act( "$n punctures your armor with a powerful blow!",
	 ch, NULL, victim, TO_VICT );
    act( "$n punctures $N's armor with a powerful blow!",
	 ch, NULL, victim, TO_NOTVICT );

    direct_damage(ch, victim, dam, gsn_puncture);

    af.where    = TO_AFFECTS;
    af.type     = gsn_puncture;
    af.level    = ch->level;
    af.duration = get_duration(gsn_puncture, ch->level);
    af.location = APPLY_AC;
    af.modifier = dam;
    af.bitvector = 0;
    
    affect_join(victim,&af);
    check_improve(ch,gsn_puncture,TRUE,3);
}

int dice_argument (const char *argument, char *arg)
{
    const char *pdot = strchr(argument, 'd');
    // invalic argument - expect 3d6 or similar
    if ( pdot == NULL )
    {
        strcpy(arg, argument);
        return -1;
    }
    // valid format - atoi converts initial part of string, which is just what we want
    strcpy(arg, pdot+1);
    return atoi(argument);
}
 
/*
 * Simple dice rolling command that lets you specify number of die and sides.
 */
DEF_DO_FUN(do_rolldice)
{
    char arg[MIL], buf[MSL];
    unsigned int result = 0;
    sh_int num = 0, size = 0;
 
    if (argument[0] == '\0')
    {
        send_to_char ("Roll how many dice with how many sides? [dice <num>d<size>]\n\r", ch);
        return;
    }
 
    if ((num = dice_argument (argument, arg)) < 0)
    {
        send_to_char ("Roll HOW many?\n\r", ch);
        return;
    }
 
    if (num > 255)
    {
        send_to_char ("You may only roll as many as 255 dice.\n\r", ch);
        return;
    }
 
    if (!is_number (arg) || arg[0] == '\0')
    {
        send_to_char ("How many sides?\n\r", ch);
        return;
    }
 
    size = atoi (arg);
 
    if (size > 255)
    {
    send_to_char ("You can only find up to 255 sided dice... And even that's a little absurd.\n\r", ch);
    return;
    }
 
    result = dice (num, size);
 
    sprintf (buf, "Rolldice: You roll %d on %d %d-sided dice.\n\r", result, num, size);
    send_to_char (buf, ch);
    sprintf (buf, "Rolldice: $n rolls %d on %d %d-sided dice.\n\r", result, num, size);
    act (buf, ch, NULL, NULL, TO_ROOM);
}

DEF_DO_FUN(do_strafe)
{
    CHAR_DATA *victim;
    int skill;

    if ( get_weapon_sn(ch) != gsn_bow )
    {
        send_to_char( "You need a bow to do that.\n\r", ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
	return;

    OBJ_DATA *held = get_eq_char(ch, WEAR_HOLD);
    if ( !held || held->item_type != ITEM_ARROWS )
    {
	send_to_char( "You need arrows in order to strafe.\n\r", ch );
	return;
    }

    if ( (skill = get_skill(ch, gsn_strafe)) == 0 )
    {
	send_to_char( "You don't know how to strafe.\n\r", ch );
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_strafe].beats );

    if ( !per_chance(skill) )
    {
        send_to_char( "You fumble your arrows.\n\r", ch );
        check_improve( ch, gsn_strafe, FALSE, 3 );
    }
    else
    {
        act( "You strafe toward $N rapidly firing arrows!", ch, NULL, victim, TO_CHAR );
        one_hit( ch, victim, gsn_strafe, FALSE );
        one_hit( ch, victim, gsn_strafe, FALSE );
        if ( per_chance(mastery_bonus(ch, gsn_strafe, 40, 50)) )
            one_hit( ch, victim, gsn_strafe, FALSE );
        check_improve( ch, gsn_strafe, TRUE, 3 );
    }
}

DEF_DO_FUN(do_infectious_arrow)
{
    AFFECT_DATA af;
    CHAR_DATA *victim;
    int skill, dam;

    if ( get_weapon_sn(ch) != gsn_bow )
    {
        send_to_char( "You need a bow to do that.\n\r", ch);
        return;
    }

    OBJ_DATA *held = get_eq_char(ch, WEAR_HOLD);
    if ( !held || held->item_type != ITEM_ARROWS )
    {
        send_to_char( "Without an arrow? LOL, Yeah right.\n\r",ch);
        return;
    }

    if ( (skill = get_skill(ch, gsn_infectious_arrow)) == 0)
    {
        send_to_char( "You don't know how to shoot an infectious arrow.\n\r",ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
	return;

    if (victim == ch)
    {
        send_to_char( "Sounds like a bad idea.\n\r", ch );
        return;
    }
    
    if ( is_safe(ch,victim) )
        return;
    
    check_killer(ch,victim);
    WAIT_STATE(ch, skill_table[gsn_infectious_arrow].beats);

    /* now the attack */
    if ( number_percent() > skill )
    {
        damage(ch,victim,0,gsn_infectious_arrow,DAM_DISEASE,TRUE);
        check_improve(ch,gsn_infectious_arrow,FALSE,3);
	return;
    }

    /* hit - how much dam? */
    dam = 2 + number_range( ch->level*3/4, ch->level*4/3 );

    act( "You fire an infectious arrow at $N!",
	 ch, NULL, victim, TO_CHAR );
    act( "$n fires an infectious arrow at you!",
	 ch, NULL, victim, TO_VICT );
    act( "$n fires an infectious arrow at $N!",
	 ch, NULL, victim, TO_NOTVICT );

    damage(ch, victim, dam, gsn_infectious_arrow, DAM_DISEASE, TRUE);

    af.where    = TO_AFFECTS;
    af.type     = gsn_infectious_arrow;
    af.level    = ch->level;
    af.duration = 0;
    af.location = APPLY_STR;
    af.modifier = -5;
    af.bitvector = 0;
    
    affect_join(victim,&af);
    check_improve(ch,gsn_infectious_arrow,TRUE,3);
}

DEF_DO_FUN(do_paroxysm)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance;
    
    if ((chance = get_skill(ch,gsn_paroxysm)) < 1)
    {
        send_to_char("You don't know how to do that.\n\r",ch);
        return;
    }
    
    one_argument( argument, arg );

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
	return;
    
    if (victim == ch)
    {
        send_to_char("Are you crazy?\n\r",ch);
        return;
    }
    
    if ( is_safe(ch, victim) )
        return;
    
    if ( !can_see_combat(ch, victim) )
    {
        send_to_char("You can't see them.\n\r",ch);
        return;
    }
    
    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL)
    {
        send_to_char( "You need to wield a weapon to do that.\n\r", ch );
        return;
    }

    if ( is_affected(ch, gsn_paroxysm_cooldown) )
    {
        send_to_char( "You're too exhausted from your last fit of rage.\n\r", ch);
        return;
    }

    if ( is_affected(victim, gsn_paroxysm) )
    {
        act( "$E can barely stand up as it is.",ch,NULL,victim,TO_CHAR );
        return;
    }

    /* can be used like backstab OR like circle.. */
    if ( ch->fighting != NULL || check_see_combat(victim, ch) )
    {
        chance = circle_chance(ch, victim, gsn_paroxysm);
    }
    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_paroxysm].beats );
    if (number_percent() <= chance)
    {
        AFFECT_DATA af;

        check_improve(ch,gsn_paroxysm,TRUE,3);

        if ( !one_hit( ch, victim, gsn_paroxysm, FALSE) )
            return;
	CHECK_RETURN(ch, victim);

	check_assassinate( ch, victim, obj, 6 );
	CHECK_RETURN(ch, victim);

	act("$n lashes out in a paroxysm of rage preventing you from stancing!",ch,NULL,victim,TO_VICT);
	act("Your paroxysm of rage prevents $N from maintaining a stance!",ch,NULL,victim,TO_CHAR);
	act("$n lashes out at $N preventing $m from stancing",ch,NULL,victim,TO_NOTVICT);

        af.where     = TO_AFFECTS;
        af.type      = gsn_paroxysm;
        af.level     = ch->level;
        af.duration  = 0;
        af.location  = APPLY_NONE;
        af.modifier  = 0; 
        af.bitvector = 0;
        affect_to_char(victim,&af);
        
        af.where     = TO_AFFECTS;
        af.type      = gsn_paroxysm_cooldown;
        af.duration  = 5;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char(ch,&af);

        victim->stance = 0; // now destances.
    }
    else
    {
        damage( ch, victim, 0, gsn_paroxysm, DAM_NONE,TRUE);
        check_improve(ch,gsn_paroxysm,FALSE,3);
    }
    return;
}

DEF_DO_FUN(do_fervent_rage)
{
    int cost = 250;
    int chance;
    
    if ( (chance = get_skill(ch,gsn_fervent_rage) ) < 1)
    {
        send_to_char("You don't know how to do that.\n\r",ch);
        return;
    }

    if (is_affected(ch,gsn_fervent_rage_cooldown))
    {
        send_to_char("You're still wound up from your last bout of rage.\n\r",ch);
        return;
    }
    
    if (IS_AFFECTED(ch,AFF_CALM))
    {
        send_to_char("You're feeling too mellow to do that.\n\r",ch);
        return;
    }
    
    if (ch->move < cost)
    {
        send_to_char("You don't have the stamina for that right now.\n\r",ch);
        return;
    }
    
        AFFECT_DATA af;
        
        WAIT_STATE(ch, skill_table[gsn_fervent_rage].beats);
        ch->move -= cost;
        
        send_to_char("Your veins bulge as you enter a fervent rage!\n\r",ch);
        act("$n's veins bulge as $e enters a fervent rage!",ch,NULL,NULL,TO_ROOM);
        check_improve(ch,gsn_fervent_rage,TRUE,3);
        
        af.where     = TO_AFFECTS;
        af.type      = gsn_fervent_rage;
        af.level     = ch->level;
        af.duration  = 1;
        af.location  = APPLY_NONE;
        af.modifier  = 0; 
        af.bitvector = 0;
        affect_to_char(ch,&af);
        
        af.where     = TO_AFFECTS;
        af.type      = gsn_fervent_rage_cooldown;
        af.duration  = 7;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char(ch,&af);
    
    return;
} 

DEF_DO_FUN(do_rupture)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance;

    
    if ((chance = get_skill(ch,gsn_rupture)) < 1)
    {
        send_to_char("You don't know how to do that.\n\r",ch);
        return;
    }
    
    one_argument( argument, arg );

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
	return;
    
    if (victim == ch)
    {
        send_to_char("Masochist much? I think not.\n\r",ch);
        return;
    }
    
    if ( is_safe(ch, victim) )
        return;
    
    if ( !can_see_combat(ch, victim) )
    {
        send_to_char("You can't see your opponent.\n\r",ch);
        return;
    }
    
    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL)
    {
        send_to_char( "You need to wield a weapon to do that.\n\r", ch );
        return;
    }

    if ( is_affected(victim, gsn_rupture) )
    {
        act( "$E is already hemmoraging.",ch,NULL,victim,TO_CHAR );
        return;
    }

    /* can be used like backstab OR like circle.. */
    if ( ch->fighting != NULL || check_see_combat(victim, ch) )
    {
        chance = circle_chance(ch, victim, gsn_rupture);
    }
    
    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_rupture].beats );
    if (number_percent() <= chance)
    {
        AFFECT_DATA af;

        check_improve(ch,gsn_rupture,TRUE,3);

        if ( !one_hit( ch, victim, gsn_rupture, FALSE) )
            return;
	CHECK_RETURN(ch, victim);

	check_assassinate( ch, victim, obj, 6 );
	CHECK_RETURN(ch, victim);

	act("$n ruptures your body causing serious pain!",ch,NULL,victim,TO_VICT);
	act("You rupture $N's body inflicting serious pain!",ch,NULL,victim,TO_CHAR);
	act("$n ruptures $N's body inflicting serious pain!",ch,NULL,victim,TO_NOTVICT);

        af.where     = TO_AFFECTS;
        af.type      = gsn_rupture;
        af.level     = ch->level;
        af.duration  = 1;
        af.location  = APPLY_VIT;
        af.modifier  = 0 - ch->level / 8; 
        af.bitvector = 0;
        affect_to_char(victim,&af);

    }
    else
    {
	act( "You try to rupture $N's body but can't seem to dig deep enough.", ch, NULL, victim, TO_CHAR );
        damage( ch, victim, 0, gsn_rupture, DAM_NONE,TRUE);
        check_improve(ch,gsn_rupture,FALSE,3);
    }
    
    return;
}

/*
DEF_DO_FUN(do_power_thrust)
{
    char arg[MAX_INPUT_LENGTH];
    char buf  [MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    OBJ_DATA *second;
    int comboused;
    int chance;
    
    if ((chance = get_skill(ch,gsn_power_thrust)) < 1)
    {
        send_to_char("You don't know how to do that.\n\r",ch);
        return;
    }
    
    if (ch->combo_points < 4)
    {
        send_to_char("You need 4 combo points to do that.\n\r",ch);
        return;
    }

    one_argument( argument, arg );

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
	return;
    
    if (victim == ch)
    {
        send_to_char("You can't do that.\n\r",ch);
        return;
    }
    
    if ( is_safe(ch, victim) )
        return;
    
    if ( !can_see(ch, victim) )
    {
        send_to_char("You can't see your opponent.\n\r",ch);
        return;
    }
    
    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL)
    {
        send_to_char( "You need to wield a weapon to do that.\n\r", ch );
        return;
    }


    if ( ch->fighting != NULL || check_see(victim, ch) )
    {
	chance = chance / 5 * 4 ;
	chance += (get_curr_stat(ch, STAT_STR) - get_curr_stat(victim, STAT_AGI)) / 6;
	if ( !can_see(victim, ch) )
	    chance += 20;
	if ( IS_AFFECTED(ch, AFF_HASTE) )
	    chance += 25;
	if ( IS_AFFECTED(victim, AFF_HASTE) )
	    chance -= 25;
    }
    
    check_killer( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_power_thrust].beats );
    if (number_percent() <= chance)
    {
        check_improve(ch,gsn_power_thrust,TRUE,3);

        full_dam(ch, victim, 500, gsn_power_thrust, DAM_PIERCE, TRUE);
	CHECK_RETURN(ch, victim);
	act("$n powerfully thrusts $s weapon into your body!",ch,NULL,victim,TO_VICT);
	act("You powerfully thrust your weapon into $N!",ch,NULL,victim,TO_CHAR);
	act("$n powerfully thrusts $s weapon into $N's body!",ch,NULL,victim,TO_NOTVICT);
        send_to_char("As you draw your weapon back a powerful charge of life surges through your body.",ch);

        ch->hit += ch->max_hit / 20;
        ch->hit = UMIN(ch->hit,ch->max_hit);

    }
    else
    {
    sprintf (buf, "Chance = %d", chance);
    send_to_char (buf, ch);
        send_to_char("Your power thrust misses.\n\r",ch);
        check_improve(ch,gsn_rupture,FALSE,3);
    }
    
    return;
}

*/


void do_quivering_palm( CHAR_DATA *ch, char *argument, void *vo)
{
    CHAR_DATA *victim; 
    int dam;
    int chance_hit, skill;
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA af;
    
    one_argument(argument, arg);
    
    if ( (skill=get_skill(ch, gsn_quivering_palm)) == 0 )
    {
        send_to_char( "You don't know that skill.\n\r", ch );
        return;
    }

  
    if ( ( victim = get_victim_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }
               
    if (victim == ch)
    {
	send_to_char( "Your stare at your palms, confused\n\r", ch);
	return;
    }
        
    if ( is_safe(ch,victim) )
	return;
        
    check_killer(ch,victim);
    WAIT_STATE(ch,skill_table[gsn_quivering_palm].beats);
    start_combat(ch, victim);
        
    /* check whether a blow hits and whether it stuns */
    chance_hit = dodge_adjust_chance(ch, victim, skill);
        
    /* check if the blow hits */
    if ( !per_chance(chance_hit) )
    {
        damage(ch,victim,0,gsn_quivering_palm,DAM_BASH,FALSE);
        act("Your palm quivers but nothing happens!", ch,NULL,victim,TO_CHAR);
        act("$n misses $s quivering palm strike!", ch, NULL, victim, TO_NOTVICT);
        act("You evade $n's quivering palm!", ch, NULL, victim, TO_VICT);
        check_improve(ch,gsn_quivering_palm,FALSE,2);
        return;
    } 
        
    dam = martial_damage(ch, victim, gsn_quivering_palm) * 2;

    if ( !saves_physical(victim, ch, ch->level * 3/2, DAM_BASH) )
    {
        act("You strike $N with a quivering palm, stunning $M!", ch, NULL, victim, TO_CHAR);
        act("$n attacks you with a quivering palm strike, stunning you!", ch, NULL, victim, TO_VICT);
        act("$n stuns $N with a quivering palm strike!", ch, NULL, victim, TO_NOTVICT);
        WAIT_STATE( victim, skill_table[gsn_quivering_palm].beats );
        DAZE_STATE( victim, skill_table[gsn_quivering_palm].beats * 2 );
        victim->stop += 1;
        
        // bonus effect
        if ( number_bits(2) && !IS_AFFECTED(victim, AFF_FEEBLEMIND) )
        {
            af.where     = TO_AFFECTS;
            af.type      = gsn_quivering_palm;
            af.level     = ch->level;
            af.duration  = get_duration(gsn_quivering_palm, ch->level);
            af.location  = APPLY_INT;
            af.modifier  = -1 * (ch->level/7);
            af.bitvector = AFF_FEEBLEMIND;
            affect_to_char( victim, &af );
            send_to_char( "Hard . . . to . . . think . . .\n\r", victim );
            act("$n seems even dumber than usual!",victim,NULL,NULL,TO_ROOM);
        }
        else if ( number_bits(2) && !IS_AFFECTED(victim, AFF_WEAKEN) )
        {
            af.where     = TO_AFFECTS;
            af.type      = gsn_quivering_palm;
            af.level     = ch->level;
            af.duration  = get_duration(gsn_quivering_palm, ch->level);
            af.location  = APPLY_STR;
            af.modifier  = -1 * (ch->level/7);
            af.bitvector = AFF_WEAKEN;
            affect_to_char( victim, &af );
            send_to_char( "You feel your strength slip away.\n\r", victim );
            act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM);
        }
        else
        {
            dam *= 2;
        }
    }
            
    /* deal damage */
    full_dam(ch, victim, dam, gsn_quivering_palm, DAM_BASH, TRUE);
    check_improve(ch, gsn_quivering_palm, TRUE, 2);

    return;
}


DEF_DO_FUN(do_mindflay)
{
  AFFECT_DATA af;
  CHAR_DATA *victim;
  int skill, dam, level;
  bool confuse = TRUE;

  if ( (skill = get_skill(ch,gsn_mindflay)) == 0)
  {
    send_to_char( "You concentrate really hard.. nnngh.\n\r",ch);
    return;
  }

  if ( (victim = get_combat_victim(ch, argument)) == NULL )
    return;

  if (victim == ch)
  {
    send_to_char( "You seem confused enough already.\n\r", ch );
    return;
  }

  if ( is_safe(ch,victim) )
    return;

  check_killer(ch,victim);
  WAIT_STATE(ch, skill_table[gsn_mindflay].beats);

  /* now attack */
  level = ch->level * (100 + skill) / 200;
  dam = (10 + level) * (50 + get_curr_stat(ch, STAT_INT)) / 50;

  if ( saves_spell(victim, ch, level*5/3, DAM_MENTAL) )
  {
    send_to_char( "They manage to shield their mind from you.\n\r", ch );
    send_to_char( "You feel something grappling for your mind.\n\r", victim);
    damage( ch, victim, 0, gsn_mindflay, DAM_MENTAL, FALSE);
    check_improve(ch,gsn_mindflay,FALSE,3);
    return;
  }
  
  // prepare affect
  af.where      = TO_AFFECTS;
  af.type       = gsn_feeblemind;
  af.level      = level;
  af.duration   = 1;
  af.location   = APPLY_INT;
  af.modifier   = -(level/8);
  af.bitvector  = AFF_FEEBLEMIND;

  if ( saves_spell(victim, ch, level + 10, DAM_MENTAL) )
  {
    dam /= 2;
    confuse = FALSE;
  }

  send_to_char( "You feel your mind getting flayed!\n\r", victim );
  full_dam( ch, victim, dam, gsn_mindflay, DAM_MENTAL, TRUE);

  if ( number_bits(2) == 0 )
  {
    act( "Your mind turns into jelly.", ch, NULL, victim, TO_VICT);
    act( "$N's mind turns into jelly.", ch, NULL, victim, TO_CHAR);
    affect_join(victim,&af);
    if ( confuse )
    {
      af.type = gsn_confusion;
      af.bitvector = AFF_INSANE;
      affect_join(victim,&af);
    }
  }
  
  check_improve(ch,gsn_mindflay,TRUE,3);
}

// paladin smite good/evil attack
DEF_DO_FUN(do_smite)
{
    CHAR_DATA *victim;
    int skill;

    if ( (skill = get_skill(ch, gsn_smite)) < 1 )
    {
        send_to_char("You don't know how to smite.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( victim == ch )
    {
        send_to_char("Don't be so harsh on yourself.\n\r", ch);
        return;
    }
    
    WAIT_STATE(ch, skill_table[gsn_smite].beats);

    // bonus damage is calculated in one_hit
    if ( !one_hit(ch, victim, gsn_smite, FALSE) || stop_attack(ch, victim) )
        return;
    
    // chance to dispel if fighting opposing alignment
    if ( get_align_type(ch) != get_align_type(victim) )
    {
        int dispel_chance = get_skill(ch, gsn_holy_avenger) / 2;
        // mastery allows dispel with only one step difference
        if ( IS_NEUTRAL(ch) || IS_NEUTRAL(victim) )
            dispel_chance += mastery_bonus(ch, gsn_smite, 40, 50);
        else
            dispel_chance += 50;
        if ( per_chance(dispel_chance) )
        {
            int level = ch->level * get_skill_total(ch, gsn_smite, 0.25) / 100;
            act("Your smite disrupts $N's magic defenses!", ch, NULL, victim, TO_CHAR);
            if ( saves_spell(victim, ch, level, DAM_OTHER) || !check_dispel_magic(level, victim) )
                send_to_char("You feel a brief tingling sensation.\n\r", victim);
        }
    }

    check_improve(ch, gsn_smite, TRUE, 3);
}

DEF_DO_FUN(do_power_attack)
{
    CHAR_DATA *victim;
    int skill;

    if ( (skill = get_skill(ch, gsn_power_attack)) < 1 )
    {
        send_to_char("You don't know how to power attack.\n\r",ch);
        return;
    }
    
    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;
    
    if ( victim == ch )
    {
        send_to_char("This isn't fight club.\n\r", ch);
        return;
    }
    
    WAIT_STATE(ch, skill_table[gsn_power_attack].beats);

    // bonus damage is calculated in one_hit
    one_hit(ch, victim, gsn_power_attack, FALSE);
    check_improve(ch, gsn_power_attack, TRUE, 3);
}

DEF_DO_FUN(do_inspire)
{
    AFFECT_DATA af;
    CHAR_DATA *vch;
    CHAR_DATA *target = NULL;
    bool all = FALSE;
    
    int skill = get_skill(ch, gsn_inspiring_song);
    int chance = (100 + skill) / 2;
    int cost = skill_table[gsn_inspiring_song].min_mana * 200 / (100 + skill);
    int level = ch->level * (100 + skill) / 200;
    
    if ( !skill )
    {
        send_to_char("You don't know how.\n\r", ch);
        return;
    }
    
    if ( argument[0] != '\0' )
    {
        char arg[MIL];
        one_argument(argument, arg);
        if ( !strcmp(arg, "all") )
            all = TRUE;
        else if ( (target = get_char_room(ch, arg)) == NULL )
        {
            send_to_char("Inspire whom?\n\r", ch);
            return;
        }
    }
    
    if ( ch->mana < cost )
    {
        send_to_char("You have run out of inspiration.\n\r", ch);
        return;
    }
    
    WAIT_STATE( ch, skill_table[gsn_inspiring_song].beats );

    if ( !per_chance(chance) )
    {
        ch->mana -= cost/2;
        send_to_char("Your song isn't very inspirational.\n\r", ch);
        return;
    }
        
    ch->mana -= cost;
    send_to_char("You sing an inspiring melody!\n\r", ch);
    act("$n sings an inspiring melody!", ch, NULL, NULL, TO_ROOM);
        
    af.where     = TO_AFFECTS;
    af.type      = gsn_inspiring_song;
    af.level     = level;
    af.duration  = get_duration(gsn_inspiring_song, level);
    af.bitvector = 0;

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if ( !all && !is_same_group(vch, ch) && !is_same_group(vch, target) )
            continue;

        send_to_char("You feel truly inspired.\n\r", vch);
        if ( vch != ch )
            act("Your song inspires $N.", ch, NULL, vch, TO_CHAR);
        
        affect_strip(vch, gsn_inspiring_song);

        af.modifier = 5 + level / 9;
        af.location = APPLY_STATS;
        affect_to_char(vch, &af);
        af.location = APPLY_HITROLL;
        affect_to_char(vch, &af);
        af.location = APPLY_DAMROLL;
        affect_to_char(vch, &af);
        af.modifier *= -1;
        af.location = APPLY_SAVES;
        affect_to_char(vch, &af);
        af.modifier *= 10;
        af.location = APPLY_AC;
        affect_to_char(vch, &af);
    }
}

DEF_DO_FUN(do_gaze)
{
    CHAR_DATA *victim;
    int skill = get_skill(ch, gsn_petrify);

    if ( skill == 0)
    {
        check_social( ch, "gaze", argument );
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;

    if ( victim == ch )
    {
        send_to_char("Not without a mirror. And better not at all.\n\r", ch);
        return;
    }
    
    if ( !can_see_combat(ch, victim) )
    {
        send_to_char("You need to see them to gaze at them.\n\r", ch);
        return;
    }

    /*
    if ( !can_see_combat(victim, ch) )
    {
        send_to_char("They cannot see you.\n\r", ch);
        return;
    }
    */

    if ( IS_AFFECTED(victim, AFF_PETRIFIED) )
    {
        send_to_char("They are already petrified.\n\r", ch);
        return;
    }

    if ( is_safe(ch,victim) )
        return;

    // ok, it's happening
    WAIT_STATE(ch, skill_table[gsn_petrify].beats);
    act("You focus your gaze on $N.", ch, NULL, victim, TO_CHAR);
    act("$n focuses $s gaze on you.", ch, NULL, victim, TO_VICT);
    act("$n focuses $s gaze on $N.", ch, NULL, victim, TO_NOTVICT);

    check_killer(ch, victim);
    start_combat(ch, victim);
    if ( per_chance(skill) )
        check_petrify(ch, victim);
}

DEF_DO_FUN(do_blast)
{
    CHAR_DATA *victim;
    int skill = get_skill(ch, gsn_eldritch_blast);

    if ( skill == 0 )
    {
        send_to_char("You don't know how to channel raw eldritch power.\n\r", ch);
        return;
    }

    if ( (victim = get_combat_victim(ch, argument)) == NULL )
        return;

    if ( is_safe(ch, victim) )
        return;

    // ok, it's happening
    WAIT_STATE(ch, skill_table[gsn_eldritch_blast].beats);
    
    if ( !per_chance(skill) )
    {
        send_to_char("You lose control of the eldritch power surging through you.\n\r", ch);
        return;
    }
    
    check_killer(ch, victim);
    start_combat(ch, victim);
    
    int dam = dice(2,4) + ch->level + ch->max_mana / 40;
    dam += dam * get_focus_bonus(ch) / 100;
    dam += get_spell_bonus_damage(ch, skill_table[gsn_eldritch_blast].beats, FALSE);
    
    bool saved = saves_spell(victim, ch, ch->level, DAM_OTHER);
    
    if ( saved || IS_AFFECTED(victim, AFF_SANCTUARY) )
        dam /= 2;
    direct_damage(ch, victim, dam, gsn_eldritch_blast);

    if ( !saved )
        eldritch_curse(ch, victim);
}
        
void eldritch_curse( CHAR_DATA *ch, CHAR_DATA *victim )
{    
    AFFECT_DATA af;

    if ( ch == victim || !check_skill(ch, gsn_eldritch_curse) )
        return;
    
    af.where     = TO_VULN;
    af.type      = gsn_eldritch_curse;
    af.level     = ch->level;
    af.duration  = get_duration(gsn_eldritch_curse, ch->level);
    af.location  = APPLY_SAVES;
    af.modifier  = dice(2,4);
    af.bitvector = VULN_MAGIC;
    affect_join(victim, &af);
    
    act_gag("You are afflicted with an eldritch curse!", victim, NULL, NULL, TO_CHAR, GAG_EFFECT);
    act_gag("$n is afflicted with an eldritch curse!", victim, NULL, NULL, TO_ROOM, GAG_EFFECT);
}

DEF_DO_FUN(do_bomb)
{
    CHAR_DATA *victim, *vnext;
    OBJ_DATA *obj;
    int dam;
    bool bomb_area = FALSE;
    
    if ( !strcmp(argument, "ground") )
    {
        bomb_area = TRUE;
        victim = NULL;
    }
    else
    {
        if ( (victim = get_combat_victim(ch, argument)) == NULL )
            return;

        if ( is_safe(ch, victim) )
            return;
    }
    
    // find lit bomb
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
        if ( obj->item_type == ITEM_EXPLOSIVE && obj->timer > 0 )
            break;
        
    if ( obj == NULL )
    {
        // find unlit bomb
        for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
            if ( obj->item_type == ITEM_EXPLOSIVE )
                break;
        // ignite if we have one
        if ( obj )
        {
            send_to_char("Finding no lit explosives, you try to ignite some.\n\r", ch);
            do_ignite(ch, "");
            return;
        }
        send_to_char("You have run out of explosives.\n\r", ch);
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);
    dam = dice(obj->value[0], obj->value[1]);

    if ( bomb_area )
    {
        act("You throw $p and dive for cover.", ch, obj, NULL, TO_CHAR);
        act("$n throws $p and dives for cover.", ch, obj, NULL, TO_ROOM);
        dam = dam * 0.75 * AREA_SPELL_FACTOR;
        for ( victim = ch->in_room->people; victim != NULL; victim = vnext)
        {
            vnext = victim->next_in_room;
            if ( !is_safe_spell(ch, victim, TRUE) )
                deal_bomb_damage(ch, victim, dam);
        }
    }
    else
    {
        act("You throw $p at $N.", ch, obj, victim, TO_CHAR);
        act("$n throws $p at you.", ch, obj, victim, TO_VICT);
        act("$n throws $p at $N.", ch, obj, victim, TO_NOTVICT);
        deal_bomb_damage(ch, victim, dam);
    }
    
    extract_obj(obj);
}
