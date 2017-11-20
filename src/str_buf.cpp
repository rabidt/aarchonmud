#include <string>

extern "C" {
#include "merc.h"
}


struct str_buf_type
{
    str_buf_type(const char *file_, const char *func_, int line_)
        : str()
        , file(file_)
        , func(func_)
        , line(line_)
        , next(nullptr)
        , prev(nullptr)
    {
        instance_count++;
        this->next = head;
        if (nullptr != head)
        {
            head->prev = this;
        }
        head = this;
    }

    ~str_buf_type()
    {
        instance_count--;

        if (nullptr != this->next)
        {
            this->next->prev = this->prev;
        }

        if (nullptr != this->prev)
        {
            this->prev->next = this->next;
        }
        else
        {
            head = this->next;
        }
    }
    
    str_buf_type(const str_buf_type &) = delete;
    str_buf_type operator=(const str_buf_type &) = delete;

    std::string str;
    std::string file;
    std::string func;
    int line;
    str_buf_type *next;
    str_buf_type *prev;

    static int instance_count;
    static str_buf_type *head;
};

str_buf_type *str_buf_type::head(nullptr);
int str_buf_type::instance_count(0);

int get_buf_count( void )
{
    return str_buf_type::instance_count;
}

void print_buf_debug(char *out, size_t sz)
{
    size_t ind = 0;
    BUFFER *curr = str_buf_type::head;
    int i;

    out[0] = '\0';

    while ( nullptr != curr )
    {
        ++i;
        int rc = snprintf(out + ind, sz - ind, 
            "%s::%s::%d\n\r",
            curr->file.c_str(),
            curr->func.c_str(),
            curr->line);

        if (rc < 0)
        {
            return;
        }

        ind += static_cast<size_t>(rc);

        if (ind >= sz)
        {
            return;
        }

        curr = curr->next;
    }
}

BUFFER *new_buf_trace( const char *file, const char *func, int line )
{
    return new BUFFER(file, func, line);
}

void free_buf( BUFFER *buf )
{
    delete buf;
}

bool add_buf( BUFFER *buf, const char *str )
{
    buf->str += str;

    return true;
}

void clear_buf( BUFFER *buf )
{
    buf->str.clear();
}

const char *buf_string( BUFFER *buf )
{
    return buf->str.c_str();
}

bool addf_buf( BUFFER *buffer, const char *fmt, ... )
{
    char buf [2*MSL];
    va_list args;
    va_start (args, fmt);
    vsnprintf (buf, sizeof(buf), fmt, args);
    va_end (args);
    
    return add_buf(buffer, buf);
}

bool addf_buf_pad(BUFFER *buffer, int pad_length, const char *fmt, ...)
{
    char buf [2*MSL];
    int rc;
    va_list args;
    va_start (args, fmt);
    rc = vsnprintf (buf, sizeof(buf), fmt, args);
    va_end (args);

    // pad
    int len = strlen_color(buf);
    if ( (rc >= 0) && 
         (static_cast<size_t>(rc) < (sizeof(buf) - 1)) && 
         (len < pad_length) )
    {
        size_t off = static_cast<size_t>(rc);
        snprintf(buf + off, 
            sizeof(buf) - off, 
            "%*s", pad_length - len, "");
    }
        
    return add_buf(buffer, buf);
}
