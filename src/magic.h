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
#ifndef MAGIC_H
#define MAGIC_H

/*
 * Spell functions.
 * Defined in magic.c.
 */
DECLARE_SPELL_FUN(  spell_null      );
DECLARE_SPELL_FUN(  spell_acid_blast    );
DECLARE_SPELL_FUN(  spell_armor     );
DECLARE_SPELL_FUN(  spell_astarks_rejuvenation  );
DECLARE_SPELL_FUN(  spell_bless     );
DECLARE_SPELL_FUN(  spell_blindness     );
DECLARE_SPELL_FUN(  spell_breath_of_god );
DECLARE_SPELL_FUN(  spell_burning_hands );
DECLARE_SPELL_FUN(  spell_call_lightning    );
DECLARE_SPELL_FUN(  spell_calm      );
DECLARE_SPELL_FUN(  spell_cancellation  );
DECLARE_SPELL_FUN(  spell_cause_harm    );
DECLARE_SPELL_FUN(  spell_change_sex    );
DECLARE_SPELL_FUN(  spell_chain_lightning   );
DECLARE_SPELL_FUN(  spell_charm_person  );
DECLARE_SPELL_FUN(  spell_chill_touch   );
DECLARE_SPELL_FUN(  spell_colour_spray  );
DECLARE_SPELL_FUN(  spell_continual_light   );
DECLARE_SPELL_FUN(  spell_control_weather   );
DECLARE_SPELL_FUN(  spell_create_bomb   );
DECLARE_SPELL_FUN(  spell_create_food   );
DECLARE_SPELL_FUN(  spell_create_rose   );
DECLARE_SPELL_FUN(  spell_create_spring );
DECLARE_SPELL_FUN(  spell_create_water  );
DECLARE_SPELL_FUN(  spell_cure_blindness    );
DECLARE_SPELL_FUN(  spell_cure_critical );
DECLARE_SPELL_FUN(  spell_cure_disease  );
DECLARE_SPELL_FUN(  spell_cure_light    );
DECLARE_SPELL_FUN(  spell_cure_mental   );
DECLARE_SPELL_FUN(  spell_cure_poison   );
DECLARE_SPELL_FUN(  spell_cure_serious  );
DECLARE_SPELL_FUN(  spell_curse     );
DECLARE_SPELL_FUN(  spell_dancing_bones );
DECLARE_SPELL_FUN(  spell_demonfire     );
DECLARE_SPELL_FUN(  spell_decompose     );
DECLARE_SPELL_FUN(  spell_detect_evil   );
DECLARE_SPELL_FUN(  spell_detect_good   );
DECLARE_SPELL_FUN(  spell_detect_hidden );
DECLARE_SPELL_FUN(  spell_detect_invis  );
DECLARE_SPELL_FUN(  spell_detect_magic  );
DECLARE_SPELL_FUN(  spell_detect_poison );
DECLARE_SPELL_FUN(  spell_dispel_evil   );
DECLARE_SPELL_FUN(  spell_dispel_good   );
DECLARE_SPELL_FUN(  spell_dispel_magic  );
DECLARE_SPELL_FUN(  spell_earthquake    );
DECLARE_SPELL_FUN(  spell_enchant_arrow );
DECLARE_SPELL_FUN(  spell_enchant_armor );
DECLARE_SPELL_FUN(  spell_enchant_weapon    );
DECLARE_SPELL_FUN(  spell_energy_drain  );
DECLARE_SPELL_FUN(  spell_extinguish    );
DECLARE_SPELL_FUN(  spell_faerie_fire   );
DECLARE_SPELL_FUN(  spell_faerie_fog    );
DECLARE_SPELL_FUN(  spell_farsight      );
DECLARE_SPELL_FUN(  spell_fireball      );
DECLARE_SPELL_FUN(  spell_fireproof     );
DECLARE_SPELL_FUN(  spell_flamestrike   );
DECLARE_SPELL_FUN(  spell_floating_disc );
DECLARE_SPELL_FUN(  spell_fly       );
DECLARE_SPELL_FUN(      spell_frenzy        );
DECLARE_SPELL_FUN(  spell_gate      );
DECLARE_SPELL_FUN(  spell_giant_strength    );
DECLARE_SPELL_FUN(  spell_hallow        );
DECLARE_SPELL_FUN(      spell_haste     );
DECLARE_SPELL_FUN(  spell_haunt         );
DECLARE_SPELL_FUN(  spell_heal          );
DECLARE_SPELL_FUN(  spell_heal_all      );
DECLARE_SPELL_FUN(  spell_heal_mind     );
DECLARE_SPELL_FUN(  spell_heat_metal    );
DECLARE_SPELL_FUN(      spell_holy_word     );
DECLARE_SPELL_FUN(  spell_identify      );
DECLARE_SPELL_FUN(  spell_infravision   );
DECLARE_SPELL_FUN(  spell_invis     );
DECLARE_SPELL_FUN(  spell_improved_invis    );
DECLARE_SPELL_FUN(  spell_know_alignment    );
DECLARE_SPELL_FUN(  spell_lightning_bolt    );
DECLARE_SPELL_FUN(  spell_locate_object );
DECLARE_SPELL_FUN(  spell_life_force    );
DECLARE_SPELL_FUN(  spell_magic_missile );
DECLARE_SPELL_FUN(  spell_mana_shield );
DECLARE_SPELL_FUN(  spell_mantra );
DECLARE_SPELL_FUN(      spell_mass_healing  );
DECLARE_SPELL_FUN(  spell_mass_invis    );
DECLARE_SPELL_FUN(  spell_nexus     );
DECLARE_SPELL_FUN(  spell_pass_door     );
DECLARE_SPELL_FUN(      spell_plague        );
DECLARE_SPELL_FUN(  spell_poison        );
DECLARE_SPELL_FUN(  spell_portal        );
DECLARE_SPELL_FUN(	spell_prayer		);
DECLARE_SPELL_FUN(  spell_protection_evil   );
DECLARE_SPELL_FUN(  spell_protection_good   );
DECLARE_SPELL_FUN(  spell_ray_of_truth  );
DECLARE_SPELL_FUN(  spell_recharge      );
DECLARE_SPELL_FUN(  spell_reflection    );
DECLARE_SPELL_FUN(  spell_refresh       );
DECLARE_SPELL_FUN(  spell_remove_curse  );
DECLARE_SPELL_FUN(  spell_replenish     );
DECLARE_SPELL_FUN(  spell_renewal       );
DECLARE_SPELL_FUN(  spell_sanctuary     );
DECLARE_SPELL_FUN(  spell_shocking_grasp    );
DECLARE_SPELL_FUN(  spell_shield        );
DECLARE_SPELL_FUN(  spell_sleep         );
DECLARE_SPELL_FUN(  spell_slow          );
DECLARE_SPELL_FUN(  spell_stone_skin    );
DECLARE_SPELL_FUN(  spell_summon        );
DECLARE_SPELL_FUN(  spell_teleport      );
DECLARE_SPELL_FUN(  spell_ventriloquate );
DECLARE_SPELL_FUN(  spell_weaken        );
DECLARE_SPELL_FUN(  spell_word_of_recall    );
DECLARE_SPELL_FUN(  spell_dowsing    );
DECLARE_SPELL_FUN(  spell_rustle_grub    );
DECLARE_SPELL_FUN(  spell_acid_breath   );
DECLARE_SPELL_FUN(  spell_fire_breath   );
DECLARE_SPELL_FUN(  spell_frost_breath  );
DECLARE_SPELL_FUN(  spell_gas_breath    );
DECLARE_SPELL_FUN(  spell_lightning_breath  );
DECLARE_SPELL_FUN(  spell_general_purpose   );
DECLARE_SPELL_FUN(  spell_high_explosive    );
DECLARE_SPELL_FUN(  spell_pacify    );
DECLARE_SPELL_FUN(  spell_feeblemind    );
DECLARE_SPELL_FUN(  spell_fear  );
DECLARE_SPELL_FUN(  spell_divine_light  );
DECLARE_SPELL_FUN(  spell_holy_binding  );
DECLARE_SPELL_FUN(  spell_detect_astral     );
DECLARE_SPELL_FUN(  spell_astral     );
DECLARE_SPELL_FUN(  spell_betray    );
DECLARE_SPELL_FUN( spell_animate_dead   );
DECLARE_SPELL_FUN( spell_ghost_chant    );
DECLARE_SPELL_FUN( spell_damned_blade   );
DECLARE_SPELL_FUN( spell_turn_undead    );
DECLARE_SPELL_FUN( spell_necrosis       );
DECLARE_SPELL_FUN( spell_dominate_soul  );
DECLARE_SPELL_FUN( spell_cannibalism    );
DECLARE_SPELL_FUN( spell_ritual_sacrifice   );
DECLARE_SPELL_FUN(  spell_cone_of_exhaustion );
DECLARE_SPELL_FUN(  spell_forboding_ooze     );
DECLARE_SPELL_FUN(  spell_tomb_stench        );
DECLARE_SPELL_FUN(  spell_zombie_breath      );
DECLARE_SPELL_FUN(  spell_zone_of_damnation  );
DECLARE_SPELL_FUN(      spell_cure_mortal       );
DECLARE_SPELL_FUN(      spell_cure_critical     );
DECLARE_SPELL_FUN(      spell_cure_serious      );
DECLARE_SPELL_FUN(      spell_cure_light        );
DECLARE_SPELL_FUN(      spell_minor_group_heal  );
DECLARE_SPELL_FUN(      spell_group_heal        );
DECLARE_SPELL_FUN(      spell_major_group_heal  );
DECLARE_SPELL_FUN(      spell_restoration       );
DECLARE_SPELL_FUN(      spell_angel_smite       );
DECLARE_SPELL_FUN(      spell_intimidation      ); 
DECLARE_SPELL_FUN(      spell_call_sidekick     );  
DECLARE_SPELL_FUN(      spell_immolation     );  
DECLARE_SPELL_FUN(      spell_epidemic     );  
DECLARE_SPELL_FUN(      spell_electrocution     );  
DECLARE_SPELL_FUN(      spell_absolute_zero     );  
DECLARE_SPELL_FUN(      spell_fade     );  
DECLARE_SPELL_FUN(      spell_protection_magic     );  
DECLARE_SPELL_FUN(      spell_hand_of_siva     );  
DECLARE_SPELL_FUN(      spell_goodberry     );  
DECLARE_SPELL_FUN(      spell_monsoon   );
DECLARE_SPELL_FUN(  spell_hailstorm );
DECLARE_SPELL_FUN(  spell_meteor_swarm );
DECLARE_SPELL_FUN(      spell_tree_golem             );     
DECLARE_SPELL_FUN(      spell_water_elemental        );     
DECLARE_SPELL_FUN(      spell_windwar                );   
DECLARE_SPELL_FUN(      spell_pass_without_trace     );
DECLARE_SPELL_FUN(      spell_entangle               );
DECLARE_SPELL_FUN(      spell_confusion               );
DECLARE_SPELL_FUN(      spell_sticks_to_snakes       );
DECLARE_SPELL_FUN(      spell_hand_of_god            );
DECLARE_SPELL_FUN(      spell_laughing_fit            );
DECLARE_SPELL_FUN(      spell_mass_confusion          );
DECLARE_SPELL_FUN(      spell_heroism                 );
DECLARE_SPELL_FUN(      spell_deaths_door             );
DECLARE_SPELL_FUN(      spell_mana_heal               );
DECLARE_SPELL_FUN(      spell_blessed_darkness         );
DECLARE_SPELL_FUN(      spell_glyph_of_evil            );
DECLARE_SPELL_FUN(      spell_tomb_rot                 );
DECLARE_SPELL_FUN(      spell_soreness                 );
DECLARE_SPELL_FUN(	spell_mephistons_scrutiny	); 
DECLARE_SPELL_FUN(	spell_rimbols_invocation	);
DECLARE_SPELL_FUN(	spell_quirkys_insanity		);
DECLARE_SPELL_FUN(	spell_sivas_sacrifice		);
DECLARE_SPELL_FUN(	spell_smotes_anachronism 	);
DECLARE_SPELL_FUN(	spell_breathe_water );
DECLARE_SPELL_FUN(	spell_stop );
DECLARE_SPELL_FUN( spell_mimic );
DECLARE_SPELL_FUN( spell_mirror_image );
DECLARE_SPELL_FUN( spell_mana_burn );
DECLARE_SPELL_FUN( spell_iron_maiden );
DECLARE_SPELL_FUN( spell_solar_flare );
DECLARE_SPELL_FUN( spell_overcharge );
DECLARE_SPELL_FUN( spell_unearth );
DECLARE_SPELL_FUN( spell_shadow_shroud );
DECLARE_SPELL_FUN( spell_phase );
DECLARE_SPELL_FUN( spell_conviction );
DECLARE_SPELL_FUN( spell_basic_apparition);
DECLARE_SPELL_FUN( spell_holy_apparition);
DECLARE_SPELL_FUN( spell_phantasmal_image );
DECLARE_SPELL_FUN( spell_shroud_of_darkness );
DECLARE_SPELL_FUN( spell_minor_fade     );  
DECLARE_SPELL_FUN( spell_divine_power );
DECLARE_SPELL_FUN( spell_shadow_companion );
DECLARE_SPELL_FUN( spell_bardic_knowledge );

/**
 * constants for easy changing of spell result "failure" handling
 * each spell function returns wether it completed successfully, where success means
 * that the spell was cast, not that it "worked" when there is a chance of failure
 * a return value of FALSE means that no mana or time is spent casting
 */
#define SR_SYNTAX   FALSE   // syntax error in spell command (for spells parsing parameter strings)
#define SR_TARGET   FALSE   // targeting error not caught by general spell targeting (e.g. wrong object type)
#define SR_UNABLE   FALSE   // spell cannot be cast under current conditions (e.g. indoors, in warfare, inventory full)
#define SR_IMMUNE   TRUE    // target is immune to the spell
#define SR_AFFECTED TRUE    // target is already affected (or not affected in case of cure spells)

// every spell function should return early (after any syntax/target/unable checks) if called for checking purposes only
#define SPELL_CHECK_RETURN if (check) return TRUE;


#endif // MAGIC_H