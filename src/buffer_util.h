/* 
   Utilities for reading from a dynamic buffer;
   based on buffer.c and methods from db.c
   by Henning Koehler <koehlerh@in.tum.de>
*/

#ifndef BUFFER_UTIL_H
#define BUFFER_UTIL_H

typedef struct read_buffer_type RBUFFER;


struct read_buffer_type
{
  DBUFFER *buf;
  int next_read;
};

//typedef struct mem_file_type MEMFILE;
//moved this to merc.h

struct mem_file_type
{
  MEMFILE *next;
  DBUFFER *buf;
  char filename[MAX_INPUT_LENGTH];
  /* quick and easy way to make saving pfiles and
     storage box files at the same time not too
     complicated.
     just set storage_box when pfile is saved to mem
     then when saving pfile always just check if 
     storage_box is null, if not also save 
     storage_box
     WHAT COULD GO WRONG*/
  //MEMFILE *storage_box;
};

/* read-buffer methods */
RBUFFER* rbuffer_new( int min_size );
void rbuffer_free( RBUFFER *rbuf );
void rbuffer_reset( RBUFFER *rbuf );
RBUFFER* read_wrap_buffer( DBUFFER *buf );
void read_wrap_free( RBUFFER *rbuf );
char bgetc( RBUFFER *rbuf );
void bungetc( RBUFFER *rbuf );
bool beof( RBUFFER *rbuf );

/* memory-file methods */
MEMFILE *memfile_new(char *filename, int min_size);
void memfile_free(MEMFILE *mf);
MEMFILE* memfile_wrap_buffer( char *filename, DBUFFER *buf );
void memfile_wrap_free( MEMFILE *mf );

/* buffer <-> file handling */
DBUFFER* load_file_to_buffer( FILE *fp );
bool write_buffer_to_file( DBUFFER *buf, FILE *fp );
bool save_to_dir( MEMFILE *mf, char *dir );

#endif

