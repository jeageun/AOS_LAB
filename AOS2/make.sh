rm client_fs
rm server_fs

gcc -w -g server_fs.c `pkg-config fuse --cflags --libs` -o server_fs

gcc -w -g client_fs.c `pkg-config fuse --cflags --libs` -o client_fs
