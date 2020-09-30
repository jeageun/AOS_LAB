rm server
rm client
rm *.o 
gcc -DHAVE_CONFIG_H -I.    -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -g -O2 -MT log.o -MD -MP -MF .deps/log.Tpo -c -o log.o log.c
gcc -DHAVE_CONFIG_H -I.    -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -g -O2 -MT bbfs.o -MD -MP -MF .deps/bbfs.Tpo -c -o client.o client_fs.c
 gcc -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -g -O2   -o client client.o log.o -lfuse -pthread 


gcc -DHAVE_CONFIG_H -I.    -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -g -O2 -MT bbfs.o -MD -MP -MF .deps/bbfs.Tpo -c -o server.o server_fs.c
 gcc -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -g -O2   -o server server.o log.o -lfuse -pthread 

