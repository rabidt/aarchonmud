/* 

   Utilities for reading from a dynamic buffer;
   based on buffer.c and methods from db.c
   by Henning Koehler <koehlerh@in.tum.de>

   read-buffers are allocated with rbuffer_new and freed with rbuffer_free
   memory-files are allocated with memfile_new and freed with memfile_free

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "buffer.h"
#include "buffer_util.h"

/* allocate a new read-buffer
 */
RBUFFER* rbuffer_new( int min_size )
{
  RBUFFER *rbuf;
  DBUFFER *buf;
#if defined(SIM_DEBUG)
   log_string("rbuffer_new: start");
#endif
  buf = buffer_new(min_size);
  if (buf == NULL)
    return NULL;
  rbuf = alloc_mem(sizeof(RBUFFER));
  if (rbuf == NULL)
  {
    buffer_free(buf);
    bug("rbuffer_new: buffer allocated but no memory left for rbuffer", 0);
    return NULL;
  }
  rbuf->buf = buf;
  rbuf->next_read = 0;
#if defined(SIM_DEBUG)
   log_string("rbuffer_new: done");
#endif
  return rbuf;
}

/* free a read-buffer
 */
void rbuffer_free( RBUFFER *rbuf )
{
#if defined(SIM_DEBUG)
   log_string("rbuffer_free: start");
#endif
  if (rbuf == NULL)
  {
    bug("rbuffer_free: NULL pointer given", 0);
    return;
  }
  if (rbuf->buf == NULL)
  {
    bug("rbuffer_free: no buffer allocated", 0);
    free_mem(rbuf, sizeof(RBUFFER));
    return;
  }
  buffer_free(rbuf->buf);
  free_mem(rbuf, sizeof(RBUFFER));
#if defined(SIM_DEBUG)
   log_string("rbuffer_free: done");
#endif
}

/* reset a read-buffer to read from the start
 */
void rbuffer_reset( RBUFFER *rbuf )
{
#if defined(SIM_DEBUG)
   log_string("rbuffer_reset: start");
#endif
  if (rbuf == NULL)
  {
    bug("rbuffer_reset: NULL pointer given", 0);
    return;
  }
  rbuf->next_read = 0;
#if defined(SIM_DEBUG)
   log_string("rbuffer_reset: done");
#endif
}

/* wrap a DBUFFER with an RBUFFER
 */
RBUFFER* read_wrap_buffer( DBUFFER *buf )
{
  RBUFFER *rbuf = alloc_mem(sizeof(RBUFFER));
#if defined(BREAD_DEBUG)
   log_string("read_wrap_buffer: start");
#endif
  if (rbuf == NULL)
  {
    bug("read_wrap_buffer: out of memory", 0);
    return NULL;
  }
  rbuf->buf = buf;
  rbuf->next_read = 0;
#if defined(BREAD_DEBUG)
   log_string("read_wrap_buffer: done");
#endif
  return rbuf;
}

/* free the read-buffer without freeing the buffer it wraps
 */
void read_wrap_free( RBUFFER *rbuf )
{
#if defined(SIM_DEBUG)
   log_string("read_wrap_free: start");
#endif
  if (rbuf == NULL)
  {
    bug("read_wrap_free: NULL pointer given", 0);
    return;
  }
  free_mem(rbuf, sizeof(RBUFFER));
#if defined(SIM_DEBUG)
   log_string("read_wrap_free: done");
#endif
}

/* read a character from a buffer
 */
char bgetc( RBUFFER *rbuf )
{
  if (rbuf == NULL || rbuf->buf == NULL)
  {
    bug("bgetc: buffer not ready", 0);
    return EOF;
  }
  
  if (rbuf->next_read >= rbuf->buf->len)
    return EOF;
  else
    return rbuf->buf->data[rbuf->next_read++];
}

/* resets the read buffer back one character
 */
void bungetc( RBUFFER *rbuf )
{
  if (rbuf == NULL || rbuf->next_read <= 0)
    bug("bungetc: buffer not ready", 0);
  else
    rbuf->next_read--;
}

/* returns wether at end of buffer (like feof)
 */
bool beof( RBUFFER *rbuf )
{
#if defined(BREAD_DEBUG)
   log_string("beof: start");
#endif
 if (rbuf == NULL || rbuf->buf == NULL)
  {
    bug("bgetc: buffer not ready", 0);
    return TRUE;
  }
#if defined(BREAD_DEBUG)
   log_string("beof: done");
#endif
  return (rbuf->next_read >= rbuf->buf->len);
}

/* allocate a new memory-file
 */
MEMFILE *memfile_new( char *filename, int min_size )
{
  MEMFILE *mf;
  DBUFFER *buf;
#if defined(SIM_DEBUG)
   log_string("memfile_new: start");
#endif
  buf = buffer_new(min_size);
  if (buf == NULL)
    return NULL;
  mf = alloc_mem(sizeof(MEMFILE));
  if (mf == NULL)
  {
    bug("memfile_new: buffer allocated but no memory left for memfile", 0);
    buffer_free(buf);
    return NULL;
  }
  mf->next = NULL;
  mf->buf = buf;
  strcpy(mf->filename, filename);
#if defined(SIM_DEBUG)
   log_string("memfile_new: done");
#endif
  return mf;
}

/* free a memory-file
 */
void memfile_free(MEMFILE *mf)
{
#if defined(SIM_DEBUG)
   log_string("memfile_free: start");
#endif
  if (mf == NULL)
  {
    bug("memfile_free: NULL pointer given", 0);
    return;
  }
  if (mf->buf == NULL)
  {
    bug("memfile_free: no buffer allocated", 0);
    free_mem(mf, sizeof(MEMFILE));
    return;
  }
  buffer_free(mf->buf);
  free_mem(mf, sizeof(MEMFILE));
#if defined(SIM_DEBUG)
   log_string("memfile_free: done");
#endif
}

/* wrap a DBUFFER with a MEMFILE
 */
MEMFILE* memfile_wrap_buffer( char *filename, DBUFFER *buf )
{
  MEMFILE *mf = alloc_mem(sizeof(MEMFILE));
#if defined(SIM_DEBUG)
   log_string("memfile_wrap_buffer: start");
#endif
  if (mf == NULL)
  {
    bug("memfile_wrap_buffer: out of memory", 0);
    return NULL;
  }
  mf->next = NULL;
  mf->buf = buf;
  strcpy(mf->filename, filename);
#if defined(SIM_DEBUG)
   log_string("memfile_wrap_buffer: done");
#endif
  return mf;
}

/* free the memory-file without freeing the buffer it wraps
 */
void memfile_wrap_free( MEMFILE *mf )
{
#if defined(SIM_DEBUG)
   log_string("memfile_wrap_free: start");
#endif
  if (mf == NULL)
  {
    bug("memfile_wrap_free: NULL pointer given", 0);
    return;
  }
  free_mem(mf, sizeof(MEMFILE));
#if defined(SIM_DEBUG)
   log_string("memfile_wrap_free: done");
#endif
}

/* load a file into a buffer
 */
DBUFFER* load_file_to_buffer( FILE *fp )
{
  long startPos, size, nr_read;
  DBUFFER *buf;

#if defined(SIM_DEBUG)
   log_string("load_file_to_buffer: start");
#endif
  if (fp == NULL)
  {
    bug("load_file_to_buffer: NULL pointer given", 0);
    return NULL;
  }

  // get file size
  startPos = ftell(fp);
  fseek(fp, 0L, 2); // jump to EOF
  size = ftell(fp) - startPos;
  fseek(fp, startPos, 0); // jump back

  // allocate buffer
  buf = buffer_new(size + 1);
  if (buf == NULL)
  {
    bug("load_file_to_buffer: out of memory", 0);
    return NULL;
  }
  
  // read the file in
  nr_read = (long)fread(buf->data, sizeof(char), (size_t)size, fp);
  if (nr_read != size)
  {
    bug("load_file_to_buffer: only %d chars read", nr_read);
    buffer_free(buf);
    return NULL;
  }
  buf->len = nr_read;
#if defined(SIM_DEBUG)
   log_string("load_file_to_buffer: done");
#endif
  return buf;
}

/* write a buffer into a file
 */
bool write_buffer_to_file( DBUFFER *buf, FILE *fp )
{
  int nr_written;
  char bug_str[MSL];
#if defined(SIM_DEBUG)
   log_string("write_buffer_to_file: start");
#endif
  if ( buf == NULL || buf->data == NULL || fp == NULL)
  {
    bug("write_buffer_to_file: NULL pointer given", 0);
    return FALSE;
  }
  nr_written = fwrite(buf->data, sizeof(char), (size_t)(buf->len), fp);
  if (nr_written != buf->len)
  {
    sprintf( bug_str, "write_buffer_to_file: only %d/%d chars could be written", 
	     nr_written, buf->len );
    bug( bug_str, 0 );
    return FALSE;
  }
#if defined(SIM_DEBUG)
   log_string("write_buffer_to_file: done");
#endif
  return TRUE;
}

/* save a memory-file to the given directory
 */
bool save_to_dir( MEMFILE *mf, char *dir )
{
  char strsave[MAX_INPUT_LENGTH];
  char bug_buf1[MSL];
  bool success;
  FILE *fp;
#if defined(SIM_DEBUG)
   log_string("save_to_dir: start");
#endif

  if (mf == NULL || dir == NULL)
  {
    bug("save_to_dir: NULL pointer given", 0);
    return FALSE;
  }

  sprintf(strsave, "%s%s", dir, mf->filename);
  if ( ( fp = fopen( strsave, "w" ) ) == NULL )
  {
    sprintf(bug_buf1, "save_to_dir: couldn't open %s", strsave);
    bug(bug_buf1, 0);
    return FALSE;
  }

  success = write_buffer_to_file( mf->buf, fp );
  fclose( fp );

  if (!success)
  {
    sprintf(bug_buf1, "save_to_dir: error saving %s", strsave);
    bug (bug_buf1, 0);
  }
#if defined(SIM_DEBUG)
   log_string("save_to_dir: done");
#endif
  return success;
}







