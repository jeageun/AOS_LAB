#ifndef __ARGS__
#define __ARGS__
#include <sys/stat.h>
#include <stdlib.h>
#include <fuse.h>
struct _args {
    struct stat *_stbuf;
    int _mask;
    void *_buf;
    size_t _size;
    void *_data;
    mode_t _mode;
	struct fuse_file_info *_fi;
	off_t _offset;
    struct timespec* _time
};

typedef struct _val {
	int _res;
	int _errno
}_val;
#endif
