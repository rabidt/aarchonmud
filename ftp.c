/* MUDftp module
 * (c) Copyright 1997, 1998 Erwin S. Andreasen and Oliver Jowett
 * This code may be freely redistributable, as long as  this header is left
 * intact.
 */

#include <stdlib.h> 
#if defined (WIN32)
#include <time.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <stdarg.h>

/* Define one of the below
 MERC: Will work for MERC, Envy, ROM, ROT
 */
#define MERC


#if defined(MERC)

#include <string.h>
#include "merc.h"

#define CLOSE_DESCRIPTOR(desc,explanation) close_socket(desc)
#define WRITE(desc,text) write_to_descriptor(desc->descriptor, text, 0)
#endif

#if !defined(NUL)
#define NUL '\0'
#endif

#if !defined(MSL)
#define MSL MAX_STRING_LENGTH
#define MIL MAX_INPUT_LENGTH
#endif

static void mudftp_notify(const char *fmt, ... ) {
    va_list va;
    char buf[MSL];
    
    va_start(va, fmt);
    vsprintf(buf, fmt, va);
    va_end(va);

    /* Then do something with "buf", appropriate to the current MUD base */
    /* E.g. send it over "Wiznet" or log it (not recommended) */
    /* log_string (buf); */
}

/* Called this because of conflict with ROT */
static void mudftp_str_replace(char *buf, const char *s, const char *repl) {
	char out_buf[MSL];
	char *pc, *out;
    int len = strlen(s);
    bool found = FALSE;
	
	for (pc = buf, out = out_buf; *pc && (out-out_buf) < (MSL-len-4); )
#if defined(WIN32)
        if (!_strnicmp(pc, s, len)) {
#else
		if (!strncasecmp(pc, s, len))  {
#endif
			out += sprintf(out, repl);
            pc += len;
            found = TRUE;
		}
		else
			*out++ = *pc++;

    if (found) { /* don't bother copying if we did not change anything */
        *out = NUL;
        strcpy(buf, out_buf);
    }
}


int line_count (const char *s) {
    int count = 0;

    for (; *s; s++)
        if (*s == '\n')
            count++;

    return count;
}

void greet_ftp (DESCRIPTOR_DATA *d, const char *argument) {
    d->connected = CON_FTP_AUTH;
	mudftp_notify("FTP connect from %s", d->host);
}

/* Authorization line: <username> <password> */
void handle_ftp_auth (DESCRIPTOR_DATA *d, const char *argument) {
    char name[MIL];
    DESCRIPTOR_DATA *dftp, *dftp_next;
    CHAR_DATA *ch = NULL;
	
	argument = one_argument((char*)argument, name);

    /* Find the descriptor of the connected character */
    for (dftp = descriptor_list; dftp; dftp=dftp->next) {
		if (dftp != d &&
            dftp->character &&
            !IS_NPC(dftp->character) &&
            dftp->connected >= CON_PLAYING &&
			!str_cmp(dftp->character->name, name)) {
			ch = dftp->character;
			break;
		}
	}
	
	if (!ch || (strcmp (crypt (argument, ch->pcdata->pwd), ch->pcdata->pwd))) {
		WRITE(d,"FAILED\n");
		CLOSE_DESCRIPTOR(d, "FTP authorization failure");
		mudftp_notify("FTP authorization for %s failed",name);
		return;
	}

    /* Search for old ftp connections */
    for (dftp = descriptor_list; dftp; dftp=dftp_next) {
        dftp_next = dftp->next;
        
		if (dftp != d &&
			(dftp->connected == CON_FTP_COMMAND ||
			 dftp->connected == CON_FTP_DATA) &&
			!str_cmp(dftp->username, name))
			CLOSE_DESCRIPTOR(dftp, "Old mudftp connection");
    }
    
	d->username = str_dup(name);
	WRITE(d, "OK mudFTP 2.0 ready\n");
	d->connected = CON_FTP_COMMAND;
	mudftp_notify("FTP authorization for %s@%s", name, d->host);
}

/* This algorithm is derived from the one supposedly used in Perl */
static const char *ftp_checksum(const char *string) {
    static char buf[10];
	int i = strlen(string);
    unsigned hash = 0;
    
    while(i--)
        hash = hash * 33U + *string++;
        
	sprintf(buf, "%08x", hash);
	return buf;
}

static CHAR_DATA *findFTPChar(DESCRIPTOR_DATA *d) {
    DESCRIPTOR_DATA *dftp;

    for (dftp = descriptor_list; dftp; dftp=dftp->next)
    {
        if (dftp != d &&
            dftp->character &&
            !str_cmp(dftp->character->name, d->username) &&
			dftp->pString)
            return dftp->character;
	}

	return NULL;
}

static void finish_file(DESCRIPTOR_DATA *d) {
    unsigned long temp_file;
    
    mudftp_notify("Transfer of %s done from %s@%s", d->ftp.filename, d->username, d->host);
    
    d->connected = CON_FTP_COMMAND;
    /* Put the file in its rightful spot */
    
    if (1 == sscanf(d->ftp.filename, "tmp/%lu", &temp_file))
	{
        CHAR_DATA *ch = findFTPChar(d);

		if (ch && ((unsigned long) ch->desc->pString) == temp_file)
        {
            char temp[MSL];
            char buf[MSL];

            strcpy(temp, d->ftp.data);
            smash_tilde(temp);

			sprintf(buf, "OK %s\n", ftp_checksum(temp));
			WRITE(d,buf);
            
            free_string(*ch->desc->pString);
            *ch->desc->pString = str_dup(temp);
            free_string(d->ftp.data);
            d->ftp.data = NULL;

            string_add(ch,"@"); /* Finish editing */

			return;
        }
    }

    WRITE(d,"FAILED Something went wrong\n");
	
    free_string(d->ftp.data);
    free_string(d->ftp.filename);
    
}

void handle_ftp_command (DESCRIPTOR_DATA *d, const char *argument) {
    char arg[MIL];
    const char *orig_argument = argument;
	
	argument = one_argument((char*)argument, arg);

	if (!str_cmp(arg, "noop")) {
		WRITE(d,"OK\n");
		return;
	}
	
	mudftp_notify("FTP command: '%s' from %s@%s", orig_argument, d->username, d->host);

	if (!str_cmp(arg, "push")) {
		if (d->ftp.mode != FTP_NORMAL)
		{
			WRITE(d, "ERROR Already in push mode\n");
			return;
		}

		d->ftp.mode = FTP_PUSH;
		WRITE(d, "OK Pushing you data as it arrives\n");
		return;
	}

	if (!str_cmp(arg, "stop"))	{
		CHAR_DATA *ch = findFTPChar(d);
		
		if (!ch)
			WRITE(d,"FAILED\n");
		else {
            free_string(d->ftp.data);
            d->ftp.data = NULL;

            string_add(ch,"@"); /* Finish editing */
			WRITE(d,"OK\n");
		}

		if (d->ftp.mode == FTP_PUSH_WAIT)
			d->ftp.mode = FTP_PUSH;
		
		return;
	}
	
	if (!str_cmp(arg, "put")) {
		argument = one_argument((char*)argument, arg);
		if (!argument[0] || !is_number((char*)argument) || atoi(argument) < 0)
			WRITE(d, "ERROR Missing filename or number of lines\n");
		else
		{
			d->ftp.filename = str_dup(arg);
			d->ftp.lines_left = atoi(argument);
			d->ftp.data = str_dup("");
			
			if (d->ftp.lines_left)
				d->connected = CON_FTP_DATA;
			else
				finish_file(d);
		}

		if (d->ftp.mode == FTP_PUSH_WAIT)
			d->ftp.mode = FTP_PUSH;
	}
	else if (!str_cmp(arg, "get")) {
		unsigned long temp_file;

		if (d->ftp.mode == FTP_PUSH_WAIT)
		{
			WRITE(d, "FAILED Expected STOP or PUT");
			return;
		}
		
		/* Send buffer being edited */
		if (1 == sscanf(argument, "tmp/%lu", &temp_file)) {
			CHAR_DATA *ch = findFTPChar(d);

			if (!ch || ((unsigned long) ch->desc->pString) != temp_file)
				WRITE(d,"FAILED\n");
			else { /* Write the string */
                char buf[MSL];
                char buf2[MSL];
                strcpy(buf2, *ch->desc->pString);
                mudftp_str_replace(buf2, "\r", "");

				sprintf(buf, "SENDING tmp/%lu %d %s\n", temp_file, line_count(buf2),
						ftp_checksum(buf2));
				if (!WRITE(d,buf) || !WRITE(d,buf2)) {
					CLOSE_DESCRIPTOR(d, "FTP write failure");
					return;
				}
			}
		}
		else
			WRITE(d, "FAILED Currently only tmp/file is supported\n");
	}
	else if (!str_cmp(arg, "quit"))
		CLOSE_DESCRIPTOR(d, "Quitting");
	else
		WRITE(d, "ERROR unknown command\n");
}


void handle_ftp_data (DESCRIPTOR_DATA *d, const char *argument) {
    int len_data, len_argument;
    
    len_data = strlen(d->ftp.data);
    len_argument = strlen(argument);
    
    /* Lines that overflow the buffer are silently lost */
    if (len_data + len_argument < MSL-16) {
        char buf[MSL];
        strcpy(buf, d->ftp.data);
        strcpy(buf+len_data, argument);
        
        /* All strings are \n internally */
        strcpy(buf+len_data+len_argument, "\n\r");
        free_string(d->ftp.data);
        d->ftp.data = str_dup(buf);
    }
    
    /* All of the file received? */
    if (--d->ftp.lines_left == 0)
        finish_file(d);
}

/* Try to push a string to this desc. false if we can't */
bool ftp_push(DESCRIPTOR_DATA *d) {
    DESCRIPTOR_DATA *m;
    for (m = descriptor_list; m; m=m->next)  {
        if (m->connected == CON_FTP_COMMAND &&
            m->ftp.mode == FTP_PUSH &&
            !str_cmp(m->username, d->character->name)) {
            
            char buf[MSL];
            char buf2[MSL];
            
            strcpy(buf2, *d->pString);
            mudftp_str_replace(buf2, "\r", ""); /* Never send \r to clients */
            
            sprintf(buf, "SENDING tmp/%lu %d %s\n", (unsigned long)d->pString, line_count(buf2),
                    ftp_checksum(buf2));
            
            if (!WRITE(m,buf) || !WRITE(m,buf2)) {
                CLOSE_DESCRIPTOR(m, "FTP write failure");
                return FALSE;
            }
            
            m->ftp.mode = FTP_PUSH_WAIT;
            
            return TRUE;
        }
    }
    
    return FALSE;
}
