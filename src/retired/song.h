/*
 *	Song.c and song.h originally written by Mike Smullens for Aeaea (1997),
 *	a ROM 2.4b4a based mud.  Code modelled after the ROM magic.c.
 */

#define SONG_APPLY 0
#define SONG_UPDATE 1
#define SONG_REMOVE 2

#define IS_SELF(ch,sh) ( (ch) == (sh) )
#define IS_GROUP(ch,sh) ( is_same_group( (ch), (sh) ) && ( (ch) != (sh) ) )
#define IS_ENEMY(ch,sh) ((is_same_group((ch)->fighting,(sh)))||(is_same_group((ch),(sh)->fighting)))

#define SAVES_SONG(percent) saves_song (level, singer, target, (percent) )
#define SONG_EFFECT_REMOVE affect_strip(target, sn)
#define SONG_EFFECT_ADD(loc, mod, bit) song_effect(target, sn, (loc), (mod), (bit))
#define SONG(name) bool (name) (int sn, int level, CHAR_DATA *singer, CHAR_DATA *target, int task)

DECLARE_SONG_FUN(song_pied_piper);
DECLARE_SONG_FUN(song_shafts_theme);
DECLARE_SONG_FUN(song_cacophony);
DECLARE_SONG_FUN(song_lust_life);
DECLARE_SONG_FUN(song_white_noise);

