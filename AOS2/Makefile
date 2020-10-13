all: server client

server : server_fs.c
	gcc  -D_FILE_OFFSET_BITS=64  -w -g server_fs.c `pkg-config fuse --cflags --libs` -o server_fs

client : client_fs.c
	gcc -D_FILE_OFFSET_BITS=64  -w -g client_fs.c `pkg-config fuse --cflags --libs` -o client_fs

clean:
	rm server_fs client_fs
