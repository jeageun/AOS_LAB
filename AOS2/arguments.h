#ifndef __ARGS__
#define __ARGS__
#include <sys/stat.h>
#include <stdlib.h>

struct _args {
    struct stat *_stbuf;
    int _mask;
    void *_buf;
    size_t _size;
    void *_data;
    mode_t _mode;
};


#endif
