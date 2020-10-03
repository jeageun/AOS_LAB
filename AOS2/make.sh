rm client
rm server

gcc -g server_fs.c `pkg-config fuse --cflags --libs` -o server

gcc -g client_fs.c `pkg-config fuse --cflags --libs` -o client
