#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#if !defined(WIN32)
#include <sys/time.h>
#endif
#endif
#include <stdio.h>
#include "merc.h"

#if defined(WIN32)
#define popen _popen
#define pclose _pclose
#endif

/*
 * Local functions.
 */
FILE 		*	popen		args( ( const char *command, const char *type ) );
int 			pclose	args( ( FILE *stream ) );
char		*	fgetf		args( ( char *s, int n, register FILE *iop ) );


void do_pipe( CHAR_DATA *ch, char *argument )
{
   char buf[5000];
   FILE *fp;
   
   fp = popen( argument, "r" );
   
   fgetf( buf, 5000, fp );
   
   page_to_char( buf, ch );
   
   pclose( fp );
   
   return;
}

char *fgetf( char *s, int n, register FILE *iop )
{
   register int c;
   register char *cs;
   
   c = '\0';
   cs = s;
   
   while( --n > 0 && (c = getc(iop)) != EOF)
      if ((*cs++ = c) == '\0')
         break;

   *cs = '\0';
   return((c == EOF && cs == s) ? NULL : s);
}