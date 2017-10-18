/*** buffer.h ****/
#ifndef BUFFER_H
#define BUFFER_H

/*#define BUFFER_DEBUG*/

#ifdef BUFFER_DEBUG /* Debugged version */

#define buffer_new(size)           __buffer_new (size, __FILE__, __LINE__)
#define buffer_strcat(buffer,text) __buffer_strcat (buffer, text, __FILE__, __LINE__)

DBUFFER * __buffer_new (int size, const char *file, unsigned line);
void __buffer_strcat (DBUFFER *buffer, const char *text, const char *file, unsigned line);

#else  /* not debugged version */

#define buffer_new(size)           __buffer_new (size)
#define buffer_strcat(buffer,text) __buffer_strcat (buffer,text)

DBUFFER * __buffer_new (int size);
void __buffer_strcat (DBUFFER *buffer, const char *text);

#endif


void buffer_free (DBUFFER *buffer);
void buffer_clear (DBUFFER *buffer);
int find_mem_size (int min_size);
int bprintf (DBUFFER *buffer, char *fmt, ...);
int rfprintf(FILE *f, const char *fmt, ...);
int rsnprintf(char *buf, size_t bufsz, const char *fmt, ...);

char* next_sr_buf( SR_BUF *sr_buf );

#endif // BUFFER_H