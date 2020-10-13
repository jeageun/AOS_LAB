rm client_fs
rm server_fs

gcc  -D_FILE_OFFSET_BITS=64  -w -g server_fs.c `pkg-config fuse --cflags --libs` -o server_fs

gcc -D_FILE_OFFSET_BITS=64  -w -g client_fs.c `pkg-config fuse --cflags --libs` -o client_fs
