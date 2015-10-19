/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,    *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                     *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael      *
 *  Chastain, Michael Quan, and Mitchell Tse.                  *
 *                                     *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                           *
 *                                     *
 *  Much time and thought has gone into this software and you are      *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                          *
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

/* this is a listing of all the commands and command related data */

#define COM_INGORE  1


/*
 * Structure for a command in the command lookup table.
 */
struct  cmd_type
{
	char * const    name;
	DO_FUN *        do_fun;
	sh_int      position;
	sh_int      level;
	sh_int      log;
	sh_int      show;
	bool        olc;
	bool	    charm;
};

struct pair_type
{
    char * const        first;
    char * const       second;
    bool              one_way;
};

/* the command table itself */
extern  const   struct  cmd_type    cmd_table   [];

/*
 * Command functions.
 * Defined in act_*.c (mostly).
 */
DECLARE_DO_FUN( do_action   );
DECLARE_DO_FUN( do_achievements  );
DECLARE_DO_FUN( do_advance  );
DECLARE_DO_FUN( do_affects  );
DECLARE_DO_FUN( do_afk      );
DECLARE_DO_FUN( do_alia     );
DECLARE_DO_FUN( do_alias    );
DECLARE_DO_FUN( do_allow    );
DECLARE_DO_FUN( do_answer   );
DECLARE_DO_FUN( do_appraise );
DECLARE_DO_FUN( do_areas    );
DECLARE_DO_FUN( do_as       );
DECLARE_DO_FUN( do_ascend   );
DECLARE_DO_FUN( do_ashift   );
DECLARE_DO_FUN( do_at       );
DECLARE_DO_FUN( do_attributes);
DECLARE_DO_FUN( do_auction  );
DECLARE_DO_FUN( do_authorize );
DECLARE_DO_FUN( do_autoassist   );
DECLARE_DO_FUN( do_autoexit );
DECLARE_DO_FUN( do_autogold );
DECLARE_DO_FUN( do_autolist );
DECLARE_DO_FUN( do_autoloot );
DECLARE_DO_FUN( do_autorescue);
DECLARE_DO_FUN( do_autosac  );
DECLARE_DO_FUN( do_autosplit);
DECLARE_DO_FUN( do_avatar);
DECLARE_DO_FUN( do_backstab );
DECLARE_DO_FUN( do_balance  );
DECLARE_DO_FUN( do_blackjack );
DECLARE_DO_FUN( do_circle   );
DECLARE_DO_FUN( do_classes  );
DECLARE_DO_FUN( do_bamfin   );
DECLARE_DO_FUN( do_bamfout  );
DECLARE_DO_FUN( do_ban      );
DECLARE_DO_FUN( do_bash     );
DECLARE_DO_FUN( do_berserk  );
DECLARE_DO_FUN( do_bitch    );
DECLARE_DO_FUN( do_bite     );
DECLARE_DO_FUN( do_blast    );
DECLARE_DO_FUN( do_blue     );
DECLARE_DO_FUN( do_board    );
DECLARE_DO_FUN( do_bomb     );
DECLARE_DO_FUN( do_bounty   );
DECLARE_DO_FUN( do_brandish );
DECLARE_DO_FUN( do_brawl    );
DECLARE_DO_FUN( do_brew     );
DECLARE_DO_FUN( do_brief    );
DECLARE_DO_FUN( do_browse   );
DECLARE_DO_FUN( do_bug      );
DECLARE_DO_FUN( do_buy      );
DECLARE_DO_FUN( do_calm     );
DECLARE_DO_FUN( do_cast     );
DECLARE_DO_FUN( do_channel  );
DECLARE_DO_FUN( do_channels );
DECLARE_DO_FUN( do_charge   );
DECLARE_DO_FUN( do_charloadtest);
DECLARE_DO_FUN( do_cheatlog );
DECLARE_DO_FUN( do_chop     );
DECLARE_DO_FUN( do_clanwar  );
DECLARE_DO_FUN( do_clanreport);
DECLARE_DO_FUN( do_clan_dump);
DECLARE_DO_FUN( do_clone    );
DECLARE_DO_FUN( do_close    );
DECLARE_DO_FUN( do_color    );
DECLARE_DO_FUN( do_colour   ); /* Colour Command By Lope */
DECLARE_DO_FUN( do_commands );
DECLARE_DO_FUN( do_combine  );
DECLARE_DO_FUN( do_compact  );
DECLARE_DO_FUN( do_compare  );
DECLARE_DO_FUN( do_consider );
DECLARE_DO_FUN( do_consent  );
DECLARE_DO_FUN( do_copyove  );
DECLARE_DO_FUN( do_copyover );
DECLARE_DO_FUN( do_count    );
DECLARE_DO_FUN( do_craft    );
DECLARE_DO_FUN( do_crash    );
DECLARE_DO_FUN( do_credits  );
DECLARE_DO_FUN( do_crimelist);
DECLARE_DO_FUN( do_deaf     );
DECLARE_DO_FUN( do_delet    );
DECLARE_DO_FUN( do_delete   );
DECLARE_DO_FUN( do_deposit );
DECLARE_DO_FUN( do_description  );
DECLARE_DO_FUN( do_detoxify );
DECLARE_DO_FUN( do_die );
DECLARE_DO_FUN( do_dirt     );
DECLARE_DO_FUN( do_dirs     );
DECLARE_DO_FUN( do_disable  );
DECLARE_DO_FUN( do_disarm   );
DECLARE_DO_FUN( do_disarm_trap  );
DECLARE_DO_FUN( do_disguise );
DECLARE_DO_FUN( do_distract );
DECLARE_DO_FUN( do_divorce  );
DECLARE_DO_FUN( do_donate   );
DECLARE_DO_FUN( do_double_strike);
DECLARE_DO_FUN( do_down     );
DECLARE_DO_FUN( do_dowsing  );
DECLARE_DO_FUN( do_drink    );
DECLARE_DO_FUN( do_drop     );
DECLARE_DO_FUN( do_dump     );
DECLARE_DO_FUN( do_east     );
DECLARE_DO_FUN( do_eat      );
DECLARE_DO_FUN( do_echo     );
DECLARE_DO_FUN( do_emote    );
DECLARE_DO_FUN( do_enter    );
DECLARE_DO_FUN( do_envenom  );
DECLARE_DO_FUN( do_eqset     );
DECLARE_DO_FUN( do_equipment);
DECLARE_DO_FUN( do_eqhelp   );
DECLARE_DO_FUN( do_etls     );
DECLARE_DO_FUN( do_examine  );
DECLARE_DO_FUN( do_exits    );
DECLARE_DO_FUN( do_explored );
DECLARE_DO_FUN( do_extract  );
DECLARE_DO_FUN( do_fatal_blow);
DECLARE_DO_FUN( do_feint    );
DECLARE_DO_FUN( do_fervent_rage    );
DECLARE_DO_FUN( do_fill     );
DECLARE_DO_FUN( do_finger   );
DECLARE_DO_FUN( do_firstaid );
DECLARE_DO_FUN( do_flag     );
DECLARE_DO_FUN( do_flee     );
DECLARE_DO_FUN( do_fledge   );
DECLARE_DO_FUN( do_flush    );
DECLARE_DO_FUN( do_follow   );
DECLARE_DO_FUN( do_forage   );
DECLARE_DO_FUN( do_forget   );
DECLARE_DO_FUN( do_forgive  );
DECLARE_DO_FUN( do_force    );
DECLARE_DO_FUN( do_freeze   );
DECLARE_DO_FUN( do_frfind   );
DECLARE_DO_FUN( do_ftag     );
DECLARE_DO_FUN( do_fvlist   );
DECLARE_DO_FUN( do_gag      );
DECLARE_DO_FUN( do_gain     );
DECLARE_DO_FUN( do_gametalk );
DECLARE_DO_FUN( do_gaze     );
DECLARE_DO_FUN( do_get      );
DECLARE_DO_FUN( do_give     );
DECLARE_DO_FUN( do_glance   );
DECLARE_DO_FUN( do_god      );
DECLARE_DO_FUN( do_gossip   );
DECLARE_DO_FUN( do_goto     );
DECLARE_DO_FUN( do_gouge    );
DECLARE_DO_FUN( do_grant    );
DECLARE_DO_FUN( do_gratz    );
DECLARE_DO_FUN( do_grep     );
DECLARE_DO_FUN( do_group    );
DECLARE_DO_FUN( do_groups   );
DECLARE_DO_FUN( do_gstat    );
DECLARE_DO_FUN( do_gtell    );
DECLARE_DO_FUN( do_guard    );
DECLARE_DO_FUN( do_guild    );
DECLARE_DO_FUN( do_heal     );
DECLARE_DO_FUN( do_headbutt );
DECLARE_DO_FUN( do_help     );
DECLARE_DO_FUN( do_helper   );
DECLARE_DO_FUN( do_herbs    );
DECLARE_DO_FUN( do_hide     );
DECLARE_DO_FUN( do_holylight);
DECLARE_DO_FUN( do_hpractice);
DECLARE_DO_FUN( do_hunt     );
DECLARE_DO_FUN( do_identify );
DECLARE_DO_FUN( do_ignite   );
DECLARE_DO_FUN( do_immflag  );
DECLARE_DO_FUN( do_immtalk  );
DECLARE_DO_FUN( do_incognito);
DECLARE_DO_FUN( do_info     );
DECLARE_DO_FUN( do_clantalk );
DECLARE_DO_FUN( do_imotd    );
DECLARE_DO_FUN( do_inspire  );
DECLARE_DO_FUN( do_intimidate );
DECLARE_DO_FUN( do_inventory);
DECLARE_DO_FUN( do_invis    );
DECLARE_DO_FUN( do_invite   );
DECLARE_DO_FUN( do_jail     );
DECLARE_DO_FUN( do_kick     );
DECLARE_DO_FUN( do_kill     );
DECLARE_DO_FUN( do_leadership );
DECLARE_DO_FUN( do_leg_sweep);
DECLARE_DO_FUN( do_lfind    );
DECLARE_DO_FUN( do_list     );
DECLARE_DO_FUN( do_load     );
DECLARE_DO_FUN( do_lock     );
DECLARE_DO_FUN( do_log      );
DECLARE_DO_FUN( do_look     );
DECLARE_DO_FUN( do_lore     );
DECLARE_DO_FUN( do_marry    );
DECLARE_DO_FUN( do_master   );
DECLARE_DO_FUN( do_melee    );
DECLARE_DO_FUN( do_memory   );
DECLARE_DO_FUN( do_merge    );
DECLARE_DO_FUN( do_mfind    );
DECLARE_DO_FUN( do_mindflay );
DECLARE_DO_FUN( do_progfind   );
DECLARE_DO_FUN( do_mload    );
DECLARE_DO_FUN( do_morph    );
DECLARE_DO_FUN( do_mortlag  );
DECLARE_DO_FUN( do_motd     );
DECLARE_DO_FUN( do_mset     );
DECLARE_DO_FUN( do_mstat    );
DECLARE_DO_FUN( do_mug      );
DECLARE_DO_FUN( do_murde    );
DECLARE_DO_FUN( do_murder   );
DECLARE_DO_FUN( do_music    );
DECLARE_DO_FUN( do_mwhere   );
DECLARE_DO_FUN( do_name     );
DECLARE_DO_FUN( do_net      );
DECLARE_DO_FUN( do_newbie   );
DECLARE_DO_FUN( do_new_dump );
DECLARE_DO_FUN( do_newlock  );
DECLARE_DO_FUN( do_noaccept );
DECLARE_DO_FUN( do_nocancel );
DECLARE_DO_FUN( do_nochannel);
DECLARE_DO_FUN( do_noemote  );
DECLARE_DO_FUN( do_noexp    );
DECLARE_DO_FUN( do_nofollow );
DECLARE_DO_FUN( do_nohelp   );
DECLARE_DO_FUN( do_nolocate );
DECLARE_DO_FUN( do_noloot   );
DECLARE_DO_FUN( do_nonote   );
DECLARE_DO_FUN( do_north    );
DECLARE_DO_FUN( do_northeast);
DECLARE_DO_FUN( do_northwest);
DECLARE_DO_FUN( do_noreply  );  /* purposely placed below do_north and the like, so 'nor' can be used */
DECLARE_DO_FUN( do_noshout  );
DECLARE_DO_FUN( do_nosummon );
DECLARE_DO_FUN( do_nosurrender );
DECLARE_DO_FUN( do_note     );
DECLARE_DO_FUN( do_notell   );
DECLARE_DO_FUN( do_ofind    );
DECLARE_DO_FUN( do_oload    );
DECLARE_DO_FUN( do_omni     );
DECLARE_DO_FUN( do_open     );
DECLARE_DO_FUN( do_openvlist);
DECLARE_DO_FUN( do_order    );
DECLARE_DO_FUN( do_oset     );
DECLARE_DO_FUN( do_ostat    );
DECLARE_DO_FUN( do_outfit   );
DECLARE_DO_FUN( do_owhere   );
DECLARE_DO_FUN( do_paralysis_poison  );
DECLARE_DO_FUN( do_pardon   );
DECLARE_DO_FUN( do_parole   );
DECLARE_DO_FUN( do_paroxysm );
DECLARE_DO_FUN( do_password );
DECLARE_DO_FUN( do_peace    );
DECLARE_DO_FUN( do_pecho    );
DECLARE_DO_FUN( do_peek     );
DECLARE_DO_FUN( do_penlist  );
DECLARE_DO_FUN( do_percentages );
DECLARE_DO_FUN( do_permban  );
DECLARE_DO_FUN( do_pflag    );
DECLARE_DO_FUN( do_pick     );
DECLARE_DO_FUN( do_pipe     );
DECLARE_DO_FUN( do_pkil     );
DECLARE_DO_FUN( do_pkill    );
DECLARE_DO_FUN( do_playback );
DECLARE_DO_FUN( do_pload    );
DECLARE_DO_FUN( do_infectious_arrow);
DECLARE_DO_FUN( do_punload  );
DECLARE_DO_FUN( do_pmote    );
DECLARE_DO_FUN( do_portal   );
DECLARE_DO_FUN( do_pose     );
DECLARE_DO_FUN( do_pour     );
DECLARE_DO_FUN( do_power_attack );
//DECLARE_DO_FUN( do_power_thrust );
DECLARE_DO_FUN( do_practice );
DECLARE_DO_FUN( do_prayer   );
DECLARE_DO_FUN( do_prefi    );
DECLARE_DO_FUN( do_prefix   );
DECLARE_DO_FUN( do_printlist);
DECLARE_DO_FUN( do_prompt   );
DECLARE_DO_FUN( do_puncture );
DECLARE_DO_FUN( do_punish   );
DECLARE_DO_FUN( do_purge    );
DECLARE_DO_FUN( do_put      );
DECLARE_DO_FUN( do_qlist    );
DECLARE_DO_FUN( do_qset     );
DECLARE_DO_FUN( do_quaff    );
DECLARE_DO_FUN( do_quest    );
DECLARE_DO_FUN( do_question );
DECLARE_DO_FUN( do_qui      );
DECLARE_DO_FUN( do_quiet    );
DECLARE_DO_FUN( do_quit     );
DECLARE_DO_FUN( do_quivering_palm   );
DECLARE_DO_FUN( do_quote    );
DECLARE_DO_FUN( do_rake     );
DECLARE_DO_FUN( do_rank     );
DECLARE_DO_FUN( do_read     );
DECLARE_DO_FUN( do_racelist );
DECLARE_DO_FUN( do_raceskills);
DECLARE_DO_FUN( do_reboo    );
DECLARE_DO_FUN( do_reboot   );
DECLARE_DO_FUN( do_recall   );
DECLARE_DO_FUN( do_recho    );
DECLARE_DO_FUN( do_recite   );
DECLARE_DO_FUN( do_recruit  );
DECLARE_DO_FUN( do_red      );
DECLARE_DO_FUN( do_reject   );
DECLARE_DO_FUN( do_release  );
DECLARE_DO_FUN( do_religion );
DECLARE_DO_FUN( do_religion_talk );
DECLARE_DO_FUN( do_remember );
DECLARE_DO_FUN( do_remort   );
DECLARE_DO_FUN( do_remove   );
DECLARE_DO_FUN( do_rent     );
DECLARE_DO_FUN( do_repeat   );
DECLARE_DO_FUN( do_replay   );
DECLARE_DO_FUN( do_reply    );
DECLARE_DO_FUN( do_report   );
DECLARE_DO_FUN( do_rescue   );
DECLARE_DO_FUN( do_reserve  );
DECLARE_DO_FUN( do_rest     );
DECLARE_DO_FUN( do_restore  );
DECLARE_DO_FUN( do_return   );
DECLARE_DO_FUN( do_review   );
DECLARE_DO_FUN( do_revoke   );
DECLARE_DO_FUN( do_rewards   );
DECLARE_DO_FUN( do_roleplay );
DECLARE_DO_FUN( do_rolldice );
DECLARE_DO_FUN( do_root     );
DECLARE_DO_FUN( do_rset     );
DECLARE_DO_FUN( do_rstat    );
DECLARE_DO_FUN( do_rules    );
DECLARE_DO_FUN( do_rupture  );
DECLARE_DO_FUN( do_rustle_grub );
DECLARE_DO_FUN( do_rvnum    );
DECLARE_DO_FUN( do_sacrifice);
DECLARE_DO_FUN( do_savantalk);
DECLARE_DO_FUN( do_save     );
DECLARE_DO_FUN( do_say      );
DECLARE_DO_FUN( do_scan     );
DECLARE_DO_FUN( do_score    );
DECLARE_DO_FUN( do_scout    );
DECLARE_DO_FUN( do_old_score );
DECLARE_DO_FUN( do_scribe   );
DECLARE_DO_FUN( do_scroll   );
DECLARE_DO_FUN( do_second   );
DECLARE_DO_FUN( do_sedit    );
DECLARE_DO_FUN( do_sell     );
DECLARE_DO_FUN( do_set      );
DECLARE_DO_FUN( do_setskill );
DECLARE_DO_FUN( do_shelter  );
DECLARE_DO_FUN( do_shout    );
DECLARE_DO_FUN( do_show     );
DECLARE_DO_FUN( do_showrace );
DECLARE_DO_FUN( do_showskill);
DECLARE_DO_FUN( do_showsubclass);
DECLARE_DO_FUN( do_shutdow  );
DECLARE_DO_FUN( do_shutdown );
DECLARE_DO_FUN( do_sing     );
DECLARE_DO_FUN( do_sire     );
DECLARE_DO_FUN( do_sit      );
DECLARE_DO_FUN( do_skill    );
DECLARE_DO_FUN( do_skills   );
DECLARE_DO_FUN( do_slash_throat );
DECLARE_DO_FUN( do_sla      );
DECLARE_DO_FUN( do_slay     );
DECLARE_DO_FUN( do_sleep    );
DECLARE_DO_FUN( do_smite    );
DECLARE_DO_FUN( do_smote    );
DECLARE_DO_FUN( do_sneak    );
DECLARE_DO_FUN( do_snoop    );
DECLARE_DO_FUN( do_social   );
DECLARE_DO_FUN( do_socials  );
DECLARE_DO_FUN( do_south    );
DECLARE_DO_FUN( do_southeast);
DECLARE_DO_FUN( do_southwest);
DECLARE_DO_FUN( do_spells   );
DECLARE_DO_FUN( do_spellup  );
DECLARE_DO_FUN( do_split    );
DECLARE_DO_FUN( do_sset     );
DECLARE_DO_FUN( do_stalk    );
DECLARE_DO_FUN( do_stance   );
DECLARE_DO_FUN( do_stance_list );
DECLARE_DO_FUN( do_stand    );
DECLARE_DO_FUN( do_stat     );
DECLARE_DO_FUN( do_stats    );
DECLARE_DO_FUN( do_steal    );
DECLARE_DO_FUN( do_story    );
DECLARE_DO_FUN( do_strafe   );
DECLARE_DO_FUN( do_string   );
DECLARE_DO_FUN( do_supplies );
DECLARE_DO_FUN( do_surrender);
DECLARE_DO_FUN( do_survey   );
DECLARE_DO_FUN( do_switch   );
DECLARE_DO_FUN( do_tag      );
DECLARE_DO_FUN( do_tame     );
DECLARE_DO_FUN( do_tattoo   );
DECLARE_DO_FUN( do_tell     );
DECLARE_DO_FUN( do_tick     );
DECLARE_DO_FUN( do_time     );
DECLARE_DO_FUN( do_title    );
DECLARE_DO_FUN( do_torch    );
DECLARE_DO_FUN( do_train    );
DECLARE_DO_FUN( do_transfer );
DECLARE_DO_FUN( do_trip     );
DECLARE_DO_FUN( do_trust    );
DECLARE_DO_FUN( do_try      );
DECLARE_DO_FUN( do_tumble   );
DECLARE_DO_FUN( do_turn_in  );
DECLARE_DO_FUN( do_typo     );
DECLARE_DO_FUN( do_unalias  );
DECLARE_DO_FUN( do_unlock   );
DECLARE_DO_FUN( do_up       );
DECLARE_DO_FUN( do_uppercut );
DECLARE_DO_FUN( do_value    );
DECLARE_DO_FUN( do_visible  );
DECLARE_DO_FUN( do_vlist    );
DECLARE_DO_FUN( do_vnum     );
DECLARE_DO_FUN( do_wake     );
DECLARE_DO_FUN( do_war_cry  );
DECLARE_DO_FUN( do_wear     );
DECLARE_DO_FUN( do_weather  );
DECLARE_DO_FUN( do_west     );
DECLARE_DO_FUN( do_where    );
DECLARE_DO_FUN( do_who      );
DECLARE_DO_FUN( do_whois    );
DECLARE_DO_FUN( do_wimpy    );
DECLARE_DO_FUN( do_wish     );
DECLARE_DO_FUN( do_withdraw );
DECLARE_DO_FUN( do_wizhelp  );
DECLARE_DO_FUN( do_wizlock  );
DECLARE_DO_FUN( do_wizlist  );
DECLARE_DO_FUN( do_wiznet   );
DECLARE_DO_FUN( do_worth    );
// DECLARE_DO_FUN( do_combo    );
DECLARE_DO_FUN( do_yell     );
DECLARE_DO_FUN( do_zap      );
DECLARE_DO_FUN( do_zecho    );

/* newer skills */
DECLARE_DO_FUN( do_burst );
DECLARE_DO_FUN( do_fury   );
DECLARE_DO_FUN( do_snipe  );
DECLARE_DO_FUN( do_drunken_fury);
DECLARE_DO_FUN( do_unjam);
DECLARE_DO_FUN( do_shoot_lock);
DECLARE_DO_FUN( do_stare);
DECLARE_DO_FUN( do_set_snare);
DECLARE_DO_FUN( do_aim);
DECLARE_DO_FUN( do_semiauto);
DECLARE_DO_FUN( do_fullauto);
DECLARE_DO_FUN( do_hogtie);
DECLARE_DO_FUN( do_estimate);
DECLARE_DO_FUN( do_stare);
DECLARE_DO_FUN( do_build_raft   );
DECLARE_DO_FUN( do_taxidermy    );
DECLARE_DO_FUN( do_camp_fire    );
DECLARE_DO_FUN( do_treat_weapon );
DECLARE_DO_FUN( do_fishing      );
DECLARE_DO_FUN( do_shield_bash  );
DECLARE_DO_FUN( do_choke_hold   );
DECLARE_DO_FUN( do_roundhouse   );
DECLARE_DO_FUN( do_round_swing  );
DECLARE_DO_FUN( do_hurl         );
DECLARE_DO_FUN( do_spit         );
DECLARE_DO_FUN( do_peel         );

/* meta-magic */
DECLARE_DO_FUN( do_mmcast       );
DECLARE_DO_FUN( do_ecast        );
DECLARE_DO_FUN( do_pcast        );
DECLARE_DO_FUN( do_qcast        );
DECLARE_DO_FUN( do_ccast        );
DECLARE_DO_FUN( do_permcast     );

/* warfare! */
DECLARE_DO_FUN( do_startwar     );
DECLARE_DO_FUN( do_nowar        );
DECLARE_DO_FUN( do_combat       );
DECLARE_DO_FUN( do_warstatus    );
DECLARE_DO_FUN( do_stopwar      );
DECLARE_DO_FUN( do_warsit       );

/* Mob Progs */
DECLARE_DO_FUN( do_mob      );
DECLARE_DO_FUN( do_mpdump   );
DECLARE_DO_FUN( do_mpedit   );
DECLARE_DO_FUN( do_mpstat   );

/* Obj Progs */
DECLARE_DO_FUN( do_opedit   );
DECLARE_DO_FUN( do_opdump   );

/* Area Progs */
DECLARE_DO_FUN( do_apedit   );
DECLARE_DO_FUN( do_apdump   );

/* Room Progs */
DECLARE_DO_FUN( do_rpedit   );
DECLARE_DO_FUN( do_rpdump   );

/* OLC */
DECLARE_DO_FUN( do_aedit    );
DECLARE_DO_FUN( do_alist    );
DECLARE_DO_FUN( do_asave    );
DECLARE_DO_FUN( do_medit    );
DECLARE_DO_FUN( do_oedit    );
DECLARE_DO_FUN( do_olc      );
DECLARE_DO_FUN( do_resets   );
DECLARE_DO_FUN( do_redit    );
DECLARE_DO_FUN( do_hedit    );

/* Erwin's REDIT */
DECLARE_DO_FUN( do_rlook        );
DECLARE_DO_FUN( do_rmob         );
DECLARE_DO_FUN( do_rkill        );
DECLARE_DO_FUN( do_rdrop        );
DECLARE_DO_FUN( do_rput         );
DECLARE_DO_FUN( do_rrandom      );
DECLARE_DO_FUN( do_rgive        );
DECLARE_DO_FUN( do_rwear        );
DECLARE_DO_FUN( do_rwhere       );
DECLARE_DO_FUN( do_rdoor        );
DECLARE_DO_FUN( do_findlock     );
DECLARE_DO_FUN( do_rforce       );

DECLARE_DO_FUN( do_lboard);
DECLARE_DO_FUN( do_lhistory);
DECLARE_DO_FUN( do_cmotd);
#ifdef FSTAT 
DECLARE_DO_FUN( do_fstat);
#endif
#ifdef LAG_FREE
DECLARE_DO_FUN( do_lagfree);
#endif
DECLARE_DO_FUN( do_cmotd);
DECLARE_DO_FUN( do_smith);
DECLARE_DO_FUN( do_pgrep);
DECLARE_DO_FUN( do_mprun);
DECLARE_DO_FUN( do_aprun);
DECLARE_DO_FUN( do_oprun);
DECLARE_DO_FUN( do_rprun);
DECLARE_DO_FUN( do_luai);
DECLARE_DO_FUN( do_luahelp);
DECLARE_DO_FUN( do_scriptdump);
DECLARE_DO_FUN( do_tables);
DECLARE_DO_FUN( do_luaconfig);
DECLARE_DO_FUN( do_luaquery);
DECLARE_DO_FUN( do_luareset);
DECLARE_DO_FUN( do_mudconfig);
DECLARE_DO_FUN( do_perfmon);
DECLARE_DO_FUN( do_findreset);
DECLARE_DO_FUN( do_diagnostic);
DECLARE_DO_FUN( do_path);
DECLARE_DO_FUN( do_guiconfig);
DECLARE_DO_FUN( do_changelog);
DECLARE_DO_FUN( do_ptitle);
