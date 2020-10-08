#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
//Create nMB file
//./a [Byte] [source] [Footprint]

long x = 1, y = 4, z = 7, w = 13;

long simplerand(void) {
	long t = x;
	t ^= t << 11;
	t ^= t >> 8;
	x = y;
	y = z;
	z = w;
	w ^= w >> 19;
	w ^= t;
	return w;
}

int
main(int argc,char* argv[]) {
   int fd0 = open(argv[2], O_RDWR);
   char *write_buf;
   char *read_buf;
   long max = atoi(argv[3]);// in MB
   int len =  atoi(argv[1]);
   long count= 0;
   long offset = 0;
   long nb0 =0;
   long nb1 =0;

   max = max*1024*1024;

   write_buf = malloc(len);
   read_buf = malloc(len);
   while(count < max){
        offset = (simplerand()%(len-2048));
        nb0 += pread(fd0, read_buf, 1024, offset);
        for(int k=0;k<1024;k++){
            read_buf[k]='A';
        }
        nb1 += pwrite(fd0,read_buf, 1024, offset);
        count +=1024;
   }
   close(fd0);
   printf("Wrote %ld, then Read %ld bytes\n", nb1,nb0);
   return 0;
}
