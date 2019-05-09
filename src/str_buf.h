#ifndef STR_BUF_H_
#define STR_BUF_H_

#include <stddef.h>
#include "booltype.h"


typedef struct  str_buf_type     BUFFER;

int get_buf_count( void );
void print_buf_debug( char *out, size_t sz );
#define new_buf( ) new_buf_trace(__FILE__, __func__, __LINE__)
BUFFER *new_buf_trace( const char *file, const char *func, int line );
void free_buf(BUFFER *buffer);
bool add_buf(BUFFER *buffer, const char *string );
void clear_buf(BUFFER *buffer);
const char *buf_string(BUFFER *buffer);
bool addf_buf(BUFFER *buffer, const char *fmt, ...);
bool addf_buf_pad(BUFFER *buffer, int pad_length, const char *fmt, ...);


#endif // STR_BUF_H_