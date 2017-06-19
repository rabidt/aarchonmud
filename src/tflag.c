#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "buffer_util.h"

/* basic methods */

#define BIT_IN_RANGE(bit) (0 < (bit) && (bit) < FLAG_MAX_BIT)
#define VSIZE (sizeof(long)*8)

void flag_set( tflag f, int bit )
{
    if ( !BIT_IN_RANGE( bit ) )
	bug( "flag_set: bit out of range (%d)", bit );
    else
	f[bit / 8] |= (1 << (bit % 8));
}

void flag_remove( tflag f, int bit )
{
    if ( !BIT_IN_RANGE( bit ) )
	bug( "flag_remove: bit out of range (%d)", bit );
    else
	f[bit / 8] &= ~(1 << (bit % 8));
}

bool flag_is_set( const tflag f, int bit )
{
    if ( !BIT_IN_RANGE( bit ) )
    {
	bug( "flag_is_set: bit out of range (%d)", bit );
	return FALSE;
    }
    return (f[bit / 8] & (1 << (bit % 8))) != 0;
}

void flag_toggle( tflag f, int bit )
{
    if ( flag_is_set(f, bit) )
	flag_remove(f, bit);
    else
	flag_set(f, bit);
}

void flag_clear( tflag f )
{
    int i;
    for ( i = 0; i < FLAG_MAX_BYTE; i++ )
	f[i] = 0;
}

bool flag_is_empty( const tflag f )
{
    int i;
    for ( i = 0; i < FLAG_MAX_BYTE; i++ )
	if ( f[i] != 0 )
	    return FALSE;
    return TRUE;
}

bool flag_equal( const tflag f1, const tflag f2 )
{
    int i;
    for ( i = 0; i < FLAG_MAX_BYTE; i++ )
	if ( f1[i] != f2[i] )
	    return FALSE;
    return TRUE;
}

void flag_copy( tflag target, const tflag source )
{
    int i;
    for ( i = 0; i < FLAG_MAX_BYTE; i++ )
	target[i] = source[i];
}

void flag_set_field( tflag f, const tflag f_set )
{
    int i;
    for ( i = 0; i < FLAG_MAX_BYTE; i++ )
	f[i] |= f_set[i];
} 

void flag_remove_field( tflag f, const tflag f_rem )
{
    int i;
    for ( i = 0; i < FLAG_MAX_BYTE; i++ )
	f[i] &= ~f_rem[i];
} 

void flag_set_vector( tflag f, long vector )
{
    size_t i;
    for ( i = 1; i <= VSIZE; i++ )
	if ( I_IS_SET(vector, i) )
	    flag_set( f, i );
}

void flag_remove_vector( tflag f, long vector )
{
    size_t i;
    for ( i = 1; i <= VSIZE; i++ )
	if ( I_IS_SET(vector, i) )
	    flag_remove( f, i );
}

void flag_copy_vector( tflag f, long vector )
{
    flag_clear( f );
    flag_set_vector( f, vector );
}

/* returns the first 31 bits of the flag as a vector */
/*
long flag_to_vector( tflag f )
{
    return *((unsigned long*)f) >> 1;
}
*/

/* advanced methods */

void bit_list_to_tflag( tflag f )
{
    tflag buf = { };
    int i;

    if ( f == NULL )
    {
	bug( "bit_list_to_tflag: NULL pointer", 0 );
	return;
    }

    /* convert from { AFF_1, AFF_2, .. } to (AFF_1 | AFF_2 | ..) */
    for (i = 0; i < FLAG_MAX_BYTE; i++)
	if ( f[i] > 0 )
	    flag_set(buf, f[i]);

    /* copy converted field back into f */
    for (i = 0; i < FLAG_MAX_BYTE; i++)
	f[i] = buf[i];
}

/* MUD specific methods */

/* returns the first flag set in vector (= old-style-flag) */
int flag_convert_old( long vector )
{
    tflag buf = {};
    size_t i;
    /* take special care of affected_by2 - flags */
    bool aff2_fix = (vector < 0);

    flag_set_vector( buf, vector );

    for ( i = 1; i < VSIZE; i++ )
        if ( flag_is_set(buf, i) )
        {
            if ( aff2_fix )
                return i + 32;
            else
                return i;
        }

    return 0;
}

char* print_tflag( const tflag f )
{
    static char buf[MSL];
    char nr_buf[10];
    int i;

    sprintf( buf, "<" );
    /* ignore bit 0 */
    for ( i = 1; i < FLAG_MAX_BIT; i++ )
	if ( flag_is_set( f, i ) )
	{
	    sprintf( nr_buf, "%d|", i );
	    strcat( buf, nr_buf );
	}
    /* delete last pipe */
    i = strlen( buf );
    if ( i > 1 )
	buf[i - 1] = '\0';
    strcat( buf, ">" );
    return buf;
}

#define IS_DIGIT(nr)     ('0' <= (nr) && (nr) <= '9')
#define DIGIT_VALUE(nr)  ((nr) - '0')

#ifdef FLAG_DEBUG
#define GETC(fp) buf[pos++] = getc(fp)
#else
#define GETC(fp) getc(fp)
#endif

void fread_tflag( FILE *fp, tflag f )
{
    int nr;
    char c;

#ifdef FLAG_DEBUG
    char buf[MSL];
    int pos = 0;
#endif

    /* clear f */
    flag_clear( f );

    /* kill leading blancs */
    do
	c = GETC( fp );
    while ( isspace(c) );

    /* check if old flag format */
    if ( c != '<' )
    {
	long flag;
	/* read flag */
	ungetc( c, fp );
	flag = fread_flag( fp );
	/* convert into tflag */
	flag_set_vector( f, flag );
	return;
    }

    /* read number after number till no number or pipe */
    c = GETC( fp );
    if ( c == '>' )
	return;

    while ( TRUE )
    {
	/* first char must be a digit */
	if ( !IS_DIGIT(c) )
	{
	    bug( "fread_tflag: unexpected non-digit (#%d)", (int)c );
	    return;
	}

	/* continue number reading */
	nr = DIGIT_VALUE(c);
	c = GETC( fp );
	while ( IS_DIGIT(c) )
	{
	    nr = 10 * nr + DIGIT_VALUE(c);
	    c = GETC( fp );
	}
	/* set number read as bit */
	if ( BIT_IN_RANGE(nr) )
	    flag_set( f, nr );
	else
	    bug( "fread_tflag: invalid bit (%d)", nr );

	/* more flag values? */
	if ( c == '|' )
	    c = GETC( fp );
	else if ( c == '>' )
	    break;
	else
	{
	    bug( "fread_tflag: unexpected token (#%d)", (int)c );
	    break;
	}
    }
#ifdef FLAG_DEBUG
    buf[pos] = '\0';
    logpf( "fread_tflag: read flag %s", buf );
#endif
}

void bread_tflag( RBUFFER *rbuf, tflag f )
{
    int nr;
    char c;

    /* clear f */
    flag_clear( f );

    /* kill leading blancs */
    do
	c =  bgetc( rbuf );
    while ( isspace(c) );

    /* check if old flag format */
    if ( c != '<' )
    {
        long flag;
	/* read flag */
	bungetc( rbuf );
	flag = bread_flag( rbuf );
	/* convert into tflag */
	flag_set_vector( f, flag );
	return;
    }

    /* read number after number till no number or pipe */
    c = bgetc( rbuf );
    if ( c == '>' )
	return;

    while ( TRUE )
    {
	/* first char must be a digit */
	if ( !IS_DIGIT(c) )
	{
	    bug( "fread_tflag: unexpected non-digit (#%d)", (int)c );
	    return;
	}

	/* continue number reading */
	nr = DIGIT_VALUE(c);
	c = bgetc( rbuf );
	while ( IS_DIGIT(c) )
	{
	    nr = 10 * nr + DIGIT_VALUE(c);
	    c = bgetc( rbuf );
	}
	/* set number read as bit */
	if ( BIT_IN_RANGE(nr) )
	    flag_set( f, nr );
	else
	    bug( "fread_tflag: invalid bit (%d)", nr );

	/* more flag values? */
	if ( c == '|' )
	    c = bgetc( rbuf );
	else if ( c == '>' )
	    break;
	else
	{
	    bug( "fread_tflag: unexpected token (#%d)", (int)c );
	    break;
	}
    }
}



/*
 * structure and methods for handling a bit field 
 * by Henning Koehler <koehlerh@in.tum.de>
 */
