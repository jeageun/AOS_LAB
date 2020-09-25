#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <sched.h>
#include <sys/mman.h>
#include <fcntl.h>

#define CACHE_LINE_SIZE 64

#ifndef opt_random_access
    #define opt_random_access 0
#endif 
long x = 1, y = 4, z = 7, w = 13;

struct read_format {
    uint64_t nr;
    struct {
        uint64_t value;
        uint64_t id;
    } values[/*2*/];
};



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

void do_mem_flush(char* p, int size){
   int i, j, count, outer, locality;
   int ws_base = 0;
   int max_base = ((size / CACHE_LINE_SIZE) - 512);
   for(outer = 0; outer < max_base*2; ++outer) {
      ws_base += 512;
      if( ws_base >= max_base ) {
         ws_base = 0;
      }
      for(locality = 0; locality < 1; locality++) {
         volatile char *a;
         char c;
         for(i = 0; i < 512; i++) {
            // Working set of 512 cache lines, 32KB
            a = p + ws_base + i * CACHE_LINE_SIZE;
            *a = 1;
         }
      }
   }
}


// p points to a region that is 1GB (ideally)
void do_mem_access(char* p, int size) {
	int i, j, count, outer, locality;
   int ws_base = 0;
   int max_base = ((size / CACHE_LINE_SIZE) - 512);
	for(outer = 0; outer < (1<<20); ++outer) {
      long r = simplerand() % max_base;
      // Pick a starting offset
      if( opt_random_access ) {
         ws_base = r;
      } else {
         //8 cache lines
         ws_base += 512;
         if( ws_base >= max_base ) {
            ws_base = 0;
         }
      }
      for(locality = 0; locality < 16; locality++) {
         volatile char *a;
         char c;
         for(i = 0; i < 512; i++) {
            // Working set of 512 cache lines, 32KB
            a = p + (ws_base + i) * CACHE_LINE_SIZE;
            if((i%8) == 0) {
               *a = 1;
            } else {
               c = *a;
            }
            //printf("%p\n",a);
         }
      }
   }
}




int global=0;
void foo(){
	char a;
	a='a';
	return;
}
int main()
{
	char* memory;
    char* flush_mem;
	char ch;
	FILE *fp;
	struct rusage prev_rec;
    struct rusage aft_rec;

	struct perf_event_attr pea;
	int fd1, fd2, fd3;
	uint64_t id1, id2, id3;
	uint64_t val1, val2, val3;
	char buf[4096];
	struct read_format* rf = (struct read_format*) buf;
	int i;
    
    int memfd;

	memset(&pea, 0, sizeof(struct perf_event_attr));
	pea.type = PERF_TYPE_HW_CACHE;
	pea.size = sizeof(struct perf_event_attr);
	pea.config = (PERF_COUNT_HW_CACHE_L1D) | (PERF_COUNT_HW_CACHE_OP_READ<<8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
	pea.disabled = 1;
	pea.exclude_kernel = 1;
	pea.exclude_hv = 1;
	pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
	fd1 = syscall(__NR_perf_event_open, &pea, 0, -1, -1, 0);
	//fd1 = perf_event_open(&pea, 0, -1, -1, 0);
    //printf("%lu\n", PERF_COUNT_HW_CACHE_L1D);
    //printf("%lu\n", PERF_COUNT_HW_CACHE_OP_READ);
    //printf("%lu\n", PERF_COUNT_HW_CACHE_RESULT_MISS);
    int tmp = ioctl(fd1, PERF_EVENT_IOC_ID, &id1);
    if (tmp<0) {
        printf("ERROR for fd1\n"); exit(0) ;
    }

	memset(&pea, 0, sizeof(struct perf_event_attr));
	pea.type = PERF_TYPE_HW_CACHE;
	pea.size = sizeof(struct perf_event_attr);
	pea.config = (PERF_COUNT_HW_CACHE_L1D) | (PERF_COUNT_HW_CACHE_OP_READ<<8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
	pea.disabled = 1;
	pea.exclude_kernel = 1;
	pea.exclude_hv = 1;
	pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
	fd2 = syscall(__NR_perf_event_open, &pea, 0, -1, fd1 /*!!!*/, 0);
	if (ioctl(fd2, PERF_EVENT_IOC_ID, &id2)<0) {
        printf("ERROR for fd2\n"); exit(0) ;
    }

	memset(&pea, 0, sizeof(struct perf_event_attr));
	pea.type = PERF_TYPE_HW_CACHE;
	pea.size = sizeof(struct perf_event_attr);
	pea.config = (PERF_COUNT_HW_CACHE_DTLB) | (PERF_COUNT_HW_CACHE_OP_READ<<8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
	pea.disabled = 1;
	pea.exclude_kernel = 1;
	pea.exclude_hv = 1;
	pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
	fd3 = syscall(__NR_perf_event_open, &pea, 0, -1, fd1 /*!!!*/, 0);
	if (ioctl(fd3, PERF_EVENT_IOC_ID, &id3)<0) {
        printf("ERROR for fd3\n"); exit(0) ;
    }
	int mmap_flags = 0;

/* Option PARSING */

    if(opt_random_access == 0){
        printf("SEQUENTIAL|");
    }else{
        printf("RANDOM|");
    }

    if((memfd = open("MEMFD",O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR))<0){
        printf("\nMEMFD fail\n");
        exit(0);
    }
    ftruncate(memfd,1024*1024*1024);
#ifdef ANON
    printf("ANNON|");
    mmap_flags |= MAP_ANON;
#else
    printf("NOANNON|");   
#endif

#ifdef POPULATE
    printf("POPULATE|");
    mmap_flags |= MAP_POPULATE;
#else
    printf("NOPOPULATE|");    
#endif

#ifdef SHARED
    printf("SHARED|");
    mmap_flags |= MAP_SHARED;
#else
    printf("PRIVATE|");
    mmap_flags |= MAP_PRIVATE;
#endif

#ifdef MMAP_ALLOC
    printf("MMAP_ALLOC|");
    #ifdef ANON    
    memory = mmap(NULL,1024*1024*1024,PROT_READ|PROT_WRITE,mmap_flags,-1,0);
    #else
    memory = mmap(NULL,1024*1024*1024,PROT_READ|PROT_WRITE,mmap_flags,memfd,0);
    #endif
#else  
    printf("MALLOC|");
    memory = (char*)malloc(1024*1024*1024);
#endif
    if(memory == MAP_FAILED)
    {
        fprintf(stderr,"\nMMAP FAIL\n");
        fprintf(stderr,"%d - %s\n",errno, strerror(errno));
        exit(0);
    }
#ifdef MSET 
    printf("MSET&MSYNC");
    memset(memory,0,1024*1024*1024);

    if(msync(memory,1024*1024*1024,MS_SYNC)==-1)
    {
        printf("MSYNC FAIL\n");
        exit(0);
    }
#else
    printf("");
#endif
    printf("\n");

    flush_mem = (char*)malloc(512*1024);

    unsigned long mask = 1; 
    if (sched_setaffinity(0, sizeof(mask), (cpu_set_t*)&mask) <0) 
    { printf("sched_setaffinity"); }
    do_mem_flush(flush_mem,512*1024);
    
    /***** Measurement Start *****/
    if (getrusage(RUSAGE_SELF,&prev_rec) <0)
    {
        printf("PREV_REC_ERROR");
        exit(0);
    }
    
    ioctl(fd1, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
  	ioctl(fd1, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);

	do_mem_access(memory,1024*1024*1024);

	ioctl(fd1, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

    /****** Measurement END ********/

    if (getrusage(RUSAGE_SELF,&aft_rec) <0){
        printf("AFT_REC ERROR\n");
        exit(0);
    }


	read(fd1, buf, sizeof(buf));
	for (i = 0; i < rf->nr; i++) {
		if (rf->values[i].id == id1) {
			val1 = rf->values[i].value;
		} else if (rf->values[i].id == id2) {
			val2 = rf->values[i].value;
		} else if (rf->values[i].id == id3) {
			val3 = rf->values[i].value;
		}
	}
    


    printf("utime_sec : %ld \n",aft_rec.ru_utime.tv_sec - prev_rec.ru_utime.tv_sec);
    printf("utime_usec : %ld \n",aft_rec.ru_utime.tv_usec - prev_rec.ru_utime.tv_usec);
    printf("stime_sec : %ld \n",prev_rec.ru_stime.tv_sec - prev_rec.ru_stime.tv_sec);
    printf("stime_usec : %ld \n",aft_rec.ru_stime.tv_usec - prev_rec.ru_stime.tv_usec);
    printf("maxrss : %ld \n",aft_rec.ru_maxrss - prev_rec.ru_maxrss);
    printf("minflt : %ld \n",aft_rec.ru_minflt - prev_rec.ru_minflt);
    printf("majflt : %ld \n",aft_rec.ru_majflt - prev_rec.ru_majflt);
    printf("inblock : %ld \n",aft_rec.ru_inblock - prev_rec.ru_inblock);
    printf("oublock : %ld \n",aft_rec.ru_oublock - prev_rec.ru_oublock);


	
	printf("L1D miss: %"PRIu64"\n", val1);
	printf("L1D access: %"PRIu64"\n", val2);
    printf("DTLB miss: %"PRIu64"\n",val3);
    
    
    
    free(flush_mem);
#ifdef MMAP_ALLOC
    munmap(memory,1024*1024*1024);
#else
    free(memory);
#endif

	return 0;
}

