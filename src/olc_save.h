#ifndef OLC_SAVE_H_
#define OLC_SAVE_H_

struct fwrite_flag_buf
{
    char buf[33];
};
const char *fwrite_flag( long flags, struct fwrite_flag_buf *stbuf );

#endif // OLC_SAVE_H_