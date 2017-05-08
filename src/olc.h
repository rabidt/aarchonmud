/***************************************************************************
 *  File: olc.h                                                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/
/*
 * This is a header file for all the OLC files.  Feel free to copy it into
 * merc.h if you wish.  Many of these routines may be handy elsewhere in
 * the code.  -Jason Dinkel
 */


/*
 * The version info.  Please use this info when reporting bugs.
 * It is displayed in the game by typing 'version' while editing.
 * Do not remove these from the code - by request of Jason Dinkel
 */
#define VERSION "ILAB Online Creation [Beta 1.0, ROM 2.3 modified]\n\r     Port a ROM 2.4 v1.8\n\r"

#define AUTHOR  "     By Jason(jdinkel@mines.colorado.edu)\n\r     Modified for use with ROM 2.3\n\r     By Hans Birkeland (hansbi@ifi.uio.no)\n\r     Modificado para uso en ROM 2.4b6\n\r     Por Ivan Toledo (itoledo@ctcreuna.cl)\n\r"
#define DATE    "     (Apr. 7, 1995 - ROM mod, Apr 16, 1995)\n\r     (Port a ROM 2.4 - Nov 2, 1996)\n\r     Version actual : 1.81 - Sep 14, 1998\n\r"

#define CREDITS "     Original by Surreality(cxw197@psu.edu) and Locke(locke@lm.com)"



/*
 * New typedefs.
 */
typedef bool OLC_FUN ( CHAR_DATA *ch, const char *argument );
#define DECLARE_OLC_FUN( fun )	OLC_FUN    fun


/* Command procedures needed ROM OLC */
DECLARE_DO_FUN(    do_help    );
DECLARE_DO_FUN(    do_rlook   );
DECLARE_SPELL_FUN( spell_null );





/*
 * Connected states for editor.
 */
#define ED_NONE		0
#define ED_AREA		1
#define ED_ROOM		2
#define ED_OBJECT	   3
#define ED_MOBILE	   4
#define ED_MPCODE    5
#define ED_HELP      6
#define ED_OPCODE    7
#define ED_APCODE	 8
#define ED_RPCODE    9


/*
 * Interpreter Prototypes
 */
void aedit( CHAR_DATA *ch, const char *argument );
void redit( CHAR_DATA *ch, const char *argument );
void medit( CHAR_DATA *ch, const char *argument );
void oedit( CHAR_DATA *ch, const char *argument );
void mpedit( CHAR_DATA *ch, const char *argument );
void opedit( CHAR_DATA *ch, const char *argument );
void apedit( CHAR_DATA *ch, const char *argument );
void hedit( CHAR_DATA *ch, const char *argument );

/*
 * OLC Constants
 */
#define MAX_MOB	1		/* Default maximum number for resetting mobs */



/*
 * Structure for an OLC editor command.
 */
struct olc_cmd_type
{
    const char * const	name;
    OLC_FUN *		olc_fun;
};



/*
 * Structure for an OLC editor startup command.
 */
struct	editor_cmd_type
{
    const char * const	name;
    DO_FUN *		do_fun;
};



/*
 * Utils.
 */
AREA_DATA *get_vnum_area	args ( ( int vnum ) );
AREA_DATA *get_area_data	args ( ( int vnum ) );
void add_reset			args ( ( ROOM_INDEX_DATA *room, 
				         RESET_DATA *pReset, int index ) );



/*
 * Interpreter Table Prototypes
 */
extern const struct olc_cmd_type	aedit_table[];
extern const struct olc_cmd_type	redit_table[];
extern const struct olc_cmd_type	oedit_table[];
extern const struct olc_cmd_type	medit_table[];
extern const struct olc_cmd_type	mpedit_table[];
extern const struct olc_cmd_type    opedit_table[];
extern const struct olc_cmd_type	apedit_table[];
extern const struct olc_cmd_type    rpedit_table[];
extern const struct olc_cmd_type	hedit_table[];


/*
 * Editor Commands.
 */
DECLARE_DO_FUN( do_aedit        );
DECLARE_DO_FUN( do_redit        );
DECLARE_DO_FUN( do_oedit        );
DECLARE_DO_FUN( do_medit        );
DECLARE_DO_FUN( do_mpedit	);
DECLARE_DO_FUN( do_opedit   );
DECLARE_DO_FUN( do_apedit   );
DECLARE_DO_FUN( do_hedit       );


/*
 * General Functions
 */
DECLARE_OLC_FUN( show_commands  );
DECLARE_OLC_FUN( show_help  );
DECLARE_OLC_FUN( show_version  );

bool edit_done( CHAR_DATA *ch );

/*
 * Area Editor Prototypes
 */
DECLARE_OLC_FUN( aedit_show		);
DECLARE_OLC_FUN( aedit_create		);
DECLARE_OLC_FUN( aedit_scrap		);
DECLARE_OLC_FUN( aedit_move		);
DECLARE_OLC_FUN( aedit_name		);
DECLARE_OLC_FUN( aedit_file		);
DECLARE_OLC_FUN( aedit_age		);
DECLARE_OLC_FUN( aedit_reset_time   );
/* DECLARE_OLC_FUN( aedit_recall	);       ROM OLC */
DECLARE_OLC_FUN( aedit_reset		);
DECLARE_OLC_FUN( aedit_purge		);
DECLARE_OLC_FUN( aedit_security		);
DECLARE_OLC_FUN( aedit_builder		);
DECLARE_OLC_FUN( aedit_comments        );
DECLARE_OLC_FUN( aedit_vnum		);
DECLARE_OLC_FUN( aedit_lvnum		);
DECLARE_OLC_FUN( aedit_uvnum		);
DECLARE_OLC_FUN( aedit_credits		);
DECLARE_OLC_FUN( aedit_remort		);
DECLARE_OLC_FUN( aedit_clone		);
/* Added for new areas command - Astark Dec 2012 */
DECLARE_OLC_FUN( aedit_minlevel         );
DECLARE_OLC_FUN( aedit_maxlevel        );
DECLARE_OLC_FUN( aedit_miniquests        );
/* End Astark's additions */
DECLARE_OLC_FUN( aedit_addaprog  );  /* ROM */
DECLARE_OLC_FUN( aedit_delaprog  );  /* ROM */


/*
 * Room Editor Prototypes
 */
DECLARE_OLC_FUN( redit_show		);
DECLARE_OLC_FUN( redit_create		);
DECLARE_OLC_FUN( redit_delete       );
DECLARE_OLC_FUN( redit_name		);
DECLARE_OLC_FUN( redit_desc		);
DECLARE_OLC_FUN( redit_comments    );
DECLARE_OLC_FUN( redit_ed		);
DECLARE_OLC_FUN( redit_format		);
DECLARE_OLC_FUN( redit_north		);
DECLARE_OLC_FUN( redit_south		);
DECLARE_OLC_FUN( redit_east		);
DECLARE_OLC_FUN( redit_west		);
DECLARE_OLC_FUN( redit_up		);
DECLARE_OLC_FUN( redit_down		);
DECLARE_OLC_FUN( redit_northeast	);
DECLARE_OLC_FUN( redit_southeast	);
DECLARE_OLC_FUN( redit_southwest	);
DECLARE_OLC_FUN( redit_northwest	);
DECLARE_OLC_FUN( redit_mreset		);
DECLARE_OLC_FUN( redit_oreset		);
DECLARE_OLC_FUN( redit_mlist		);
DECLARE_OLC_FUN( redit_rlist		);
DECLARE_OLC_FUN( redit_olist		);
DECLARE_OLC_FUN( redit_mshow		);
DECLARE_OLC_FUN( redit_oshow		);
DECLARE_OLC_FUN( redit_heal		);
DECLARE_OLC_FUN( redit_mana		);
DECLARE_OLC_FUN( redit_clan		);
DECLARE_OLC_FUN( redit_clan_rank	);
DECLARE_OLC_FUN( redit_owner		);
DECLARE_OLC_FUN( redit_room		);
DECLARE_OLC_FUN( redit_sector		);
DECLARE_OLC_FUN( redit_addrprog );
DECLARE_OLC_FUN( redit_delrprog );


/*
 * Object Editor Prototypes
 */
DECLARE_OLC_FUN( oedit_show		);
DECLARE_OLC_FUN( oedit_create		);
DECLARE_OLC_FUN( oedit_delete   );
DECLARE_OLC_FUN( oedit_name		);
DECLARE_OLC_FUN( oedit_short		);
DECLARE_OLC_FUN( oedit_long		);
DECLARE_OLC_FUN( oedit_comments    );
DECLARE_OLC_FUN( oedit_addaffect	);
DECLARE_OLC_FUN( oedit_addapply		);
DECLARE_OLC_FUN( oedit_delaffect	);
DECLARE_OLC_FUN( oedit_value0		);
DECLARE_OLC_FUN( oedit_value1		);
DECLARE_OLC_FUN( oedit_value2		);
DECLARE_OLC_FUN( oedit_value3		);
DECLARE_OLC_FUN( oedit_value4		);  /* ROM */
DECLARE_OLC_FUN( oedit_weight		);
DECLARE_OLC_FUN( oedit_cost		);
DECLARE_OLC_FUN( oedit_clan		);
DECLARE_OLC_FUN( oedit_rank		);
DECLARE_OLC_FUN( oedit_ed		);

DECLARE_OLC_FUN( oedit_extra            );  /* ROM */
DECLARE_OLC_FUN( oedit_wear             );  /* ROM */
DECLARE_OLC_FUN( oedit_type             );  /* ROM */
DECLARE_OLC_FUN( oedit_affect           );  /* ROM */
DECLARE_OLC_FUN( oedit_material		);  /* ROM */
DECLARE_OLC_FUN( oedit_level            );  /* ROM */
DECLARE_OLC_FUN( oedit_condition        );  /* ROM */
DECLARE_OLC_FUN( oedit_combine          );  /* ROM */
DECLARE_OLC_FUN( oedit_rating           );  /* ROM */
DECLARE_OLC_FUN( oedit_adjust           );
DECLARE_OLC_FUN( oedit_addoprog  );  /* ROM */
DECLARE_OLC_FUN( oedit_deloprog  );  /* ROM */

/*
 * Mobile Editor Prototypes
 */
DECLARE_OLC_FUN( medit_show		);
DECLARE_OLC_FUN( medit_create		);
DECLARE_OLC_FUN( medit_delete       );
DECLARE_OLC_FUN( medit_name		);
DECLARE_OLC_FUN( medit_short		);
DECLARE_OLC_FUN( medit_long		);
DECLARE_OLC_FUN( medit_shop		);
DECLARE_OLC_FUN( medit_bossachieve);
DECLARE_OLC_FUN( medit_desc		);
DECLARE_OLC_FUN( medit_comments    );
DECLARE_OLC_FUN( medit_level		);
DECLARE_OLC_FUN( medit_align		);
DECLARE_OLC_FUN( medit_spec		);

DECLARE_OLC_FUN( medit_sex       );  /* ROM */
DECLARE_OLC_FUN( medit_act       );  /* ROM */
DECLARE_OLC_FUN( medit_affect    );  /* ROM */
DECLARE_OLC_FUN( medit_ac        );  /* ROM */
DECLARE_OLC_FUN( medit_form      );  /* ROM */
DECLARE_OLC_FUN( medit_part      );  /* ROM */
DECLARE_OLC_FUN( medit_imm       );  /* ROM */
DECLARE_OLC_FUN( medit_res       );  /* ROM */
DECLARE_OLC_FUN( medit_vuln      );  /* ROM */
DECLARE_OLC_FUN( medit_off       );  /* ROM */
DECLARE_OLC_FUN( medit_size      );  /* ROM */
DECLARE_OLC_FUN( medit_race      );  /* ROM */
DECLARE_OLC_FUN( medit_position  );  /* ROM */
DECLARE_OLC_FUN( medit_damtype   );  /* ROM */
DECLARE_OLC_FUN( medit_group     );  /* ROM */
DECLARE_OLC_FUN( medit_addmprog  );  /* ROM */
DECLARE_OLC_FUN( medit_delmprog  );  /* ROM */
DECLARE_OLC_FUN( medit_stance    );
DECLARE_OLC_FUN( medit_skill     );
DECLARE_OLC_FUN( medit_hitpoints );
DECLARE_OLC_FUN( medit_mana      );
DECLARE_OLC_FUN( medit_move      );
DECLARE_OLC_FUN( medit_hitroll   );
DECLARE_OLC_FUN( medit_damage    );
DECLARE_OLC_FUN( medit_armor     );
DECLARE_OLC_FUN( medit_saves     );
DECLARE_OLC_FUN( medit_wealth    );

/* Mobprog editor */

DECLARE_OLC_FUN( mpedit_create   );
DECLARE_OLC_FUN( mpedit_delete   );
DECLARE_OLC_FUN( mpedit_code     );
DECLARE_OLC_FUN( mpedit_show     );
DECLARE_OLC_FUN( mpedit_list     );
DECLARE_OLC_FUN( mpedit_if       );
DECLARE_OLC_FUN( mpedit_mob      );
DECLARE_OLC_FUN( mpedit_lua      );
DECLARE_OLC_FUN( mpedit_security );

/* Objprog editor */
DECLARE_OLC_FUN( opedit_create   );
DECLARE_OLC_FUN( opedit_delete   );
DECLARE_OLC_FUN( opedit_code     );
DECLARE_OLC_FUN( opedit_show     );
DECLARE_OLC_FUN( opedit_security );

/* Areaprog editor */
DECLARE_OLC_FUN( apedit_create   );
DECLARE_OLC_FUN( apedit_delete   );
DECLARE_OLC_FUN( apedit_code     );
DECLARE_OLC_FUN( apedit_show     );
DECLARE_OLC_FUN( apedit_security );

/* Roomprog editor */
DECLARE_OLC_FUN( rpedit_create   );
DECLARE_OLC_FUN( rpedit_delete   );
DECLARE_OLC_FUN( rpedit_code     );
DECLARE_OLC_FUN( rpedit_show     );
DECLARE_OLC_FUN( rpedit_security );


/* Help Editor - kermit 1/98 */
DECLARE_OLC_FUN( hedit_create    );
DECLARE_OLC_FUN( hedit_show      );
DECLARE_OLC_FUN( hedit_desc      );
DECLARE_OLC_FUN( hedit_keywords  );
DECLARE_OLC_FUN( hedit_level     );
DECLARE_OLC_FUN( hedit_delete    );

DECLARE_OLC_FUN( raceedit_create );
DECLARE_OLC_FUN( raceedit_delete );
DECLARE_OLC_FUN( raceedit_show );
DECLARE_OLC_FUN( raceedit_whoname );
DECLARE_OLC_FUN( raceedit_range );
DECLARE_OLC_FUN( raceedit_etl );
DECLARE_OLC_FUN( raceedit_size );
DECLARE_OLC_FUN( raceedit_gender );
DECLARE_OLC_FUN( raceedit_complete );
DECLARE_OLC_FUN( raceedit_addskill );
DECLARE_OLC_FUN( raceedit_remskill );


/*
 * Macros
 */

/* Return pointers to what is being edited. */
#define EDIT_MOB(Ch, Mob)     ( Mob= (Ch->desc->editor == ED_MOBILE) ? \
                                     (MOB_INDEX_DATA *)Ch->desc->pEdit : \
                                     NULL )
#define EDIT_OBJ(Ch, Obj)     ( Obj = (Ch->desc->editor == ED_OBJECT) ? \
                                      (OBJ_INDEX_DATA *)Ch->desc->pEdit : \
                                     NULL )
#define EDIT_ROOM(Ch, Room)   ( Room = (Ch->desc->editor == ED_ROOM) ? \
                                        Ch->in_room : \
                                        NULL )
#define EDIT_AREA(Ch, Area)   ( Area = (Ch->desc->editor == ED_AREA) ? \
                                        (AREA_DATA *)Ch->desc->pEdit : \
                                        NULL)
#define EDIT_HELP(Ch, Help)   ( Help = (Ch->desc->editor == ED_HELP) ? \
                                        (HELP_DATA *)Ch->desc->pEdit : \
                                        NULL )
#define EDIT_MPCODE(Ch, Code) ( Code = (Ch->desc->editor == ED_MPCODE) ? \
                                        (PROG_CODE*)Ch->desc->pEdit : \
                                        NULL)
#define EDIT_OPCODE(Ch, Code) ( Code = (Ch->desc->editor == ED_OPCODE) ? \
                                        (PROG_CODE*)Ch->desc->pEdit : \
                                        NULL )
#define EDIT_APCODE(Ch, Code) ( Code = (Ch->desc->editor == ED_APCODE) ? \
                                        (PROG_CODE*)Ch->desc->pEdit : \
                                        NULL )
#define EDIT_RPCODE(Ch, Code) ( Code = (Ch->desc->editor == ED_RPCODE) ? \
                                        (PROG_CODE*)Ch->desc->pEdit : \
                                        NULL )



/*
 * Prototypes
 */
/* mem.c - memory prototypes. */
#define ED	EXTRA_DESCR_DATA
RESET_DATA	*new_reset_data		args ( ( void ) );
void		free_reset_data		args ( ( RESET_DATA *pReset ) );
AREA_DATA	*new_area		args ( ( void ) );
void		free_area		args ( ( AREA_DATA *pArea ) );
EXIT_DATA	*new_exit		args ( ( void ) );
void		free_exit		args ( ( EXIT_DATA *pExit ) );
ED 		*new_extra_descr	args ( ( void ) );
void		free_extra_descr	args ( ( ED *pExtra ) );
ROOM_INDEX_DATA *new_room_index		args ( ( void ) );
void		free_room_index		args ( ( ROOM_INDEX_DATA *pRoom ) );
AFFECT_DATA	*new_affect		args ( ( void ) );
void		free_affect		args ( ( AFFECT_DATA* pAf ) );
SHOP_DATA	*new_shop		args ( ( void ) );
void		free_shop		args ( ( SHOP_DATA *pShop ) );
OBJ_INDEX_DATA	*new_obj_index		args ( ( void ) );
void		free_obj_index		args ( ( OBJ_INDEX_DATA *pObj ) );
MOB_INDEX_DATA	*new_mob_index		args ( ( void ) );
void		free_mob_index		args ( ( MOB_INDEX_DATA *pMob ) );
BOSSACHV    *new_boss_achieve       args ( ( void ) );
void        free_boss_achieve   args ( ( BOSSACHV *pBoss ) );
#undef	ED

void		show_liqlist		args ( ( CHAR_DATA *ch ) );
void		show_damlist		args ( ( CHAR_DATA *ch ) );

char *		mprog_type_to_name	args ( ( int type ) );
PROG_LIST      *new_mprog              args ( ( void ) );
void            free_mprog              args ( ( PROG_LIST *mp ) );
PROG_CODE	*new_mpcode		args ( (void) );
void		free_mpcode		args ( ( PROG_CODE *pMcode));

PROG_LIST      *new_oprog              args ( ( void ) );
void            free_oprog              args ( ( PROG_LIST *op ) );
PROG_CODE *new_opcode      args ( (void) );
void        free_opcode     args ( ( PROG_CODE *pOcode));

PROG_LIST      *new_aprog              args ( ( void ) );
void            free_aprog              args ( ( PROG_LIST *ap ) );
PROG_CODE *new_apcode      args ( (void) );
void        free_apcode     args ( ( PROG_CODE *pAcode));

PROG_LIST      *new_rprog              args ( ( void ) );
void            free_rprog              args ( ( PROG_LIST *rp ) );
PROG_CODE *new_rpcode      args ( (void) );
void        free_rpcode     args ( ( PROG_CODE *pRcode));


HELP_DATA *new_help args ( (void) );
void free_help args ( ( HELP_DATA * pHelp));

/* olc_mpcode.c */
void fix_mprog_mobs( CHAR_DATA *ch, PROG_CODE *pMcode );

/* olc_opcode.c */
void fix_oprog_objs( CHAR_DATA *ch, PROG_CODE *pOcode );

/* olc_apcode.c */
void fix_aprog_areas( CHAR_DATA *ch, PROG_CODE *pAcode );

/* olc_rpcode.c */
void fix_rprog_rooms( CHAR_DATA *ch, PROG_CODE *pRcode );


bool is_being_edited( void *ptr );
