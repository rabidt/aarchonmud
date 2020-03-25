#ifndef TATTOO_H
#define TATTOO_H

/* merc.h
#define TATTOO_NONE -1
*/

/***************************** tattoo_list ***************************/

/* merc.h
typedef int tattoo_list[MAX_WEAR];
*/

bool is_tattoo_list_empty( tattoo_list tl );

void clear_tattoos( tattoo_list tl );
const char* print_tattoos( tattoo_list tl );
void bread_tattoos( RBUFFER *rbuf, tattoo_list tl );

/***************************** tattoo_data ***************************/

const char* tattoo_desc( int ID );

/***************************** general *******************************/ 

float tattoo_bonus_factor( float level );
float get_obj_tattoo_level( int obj_level, int level );
void tattoo_modify_equip( CHAR_DATA *ch, int loc, bool fAdd, bool drop, bool basic );
void tattoo_modify_level( CHAR_DATA *ch, int old_level, int new_level );
void tattoo_modify_reset( CHAR_DATA *ch );
int get_tattoo_ch( CHAR_DATA *ch, int loc );

/***************************** do_functions **************************/

DECLARE_DO_FUN(do_tattoo);

#endif


