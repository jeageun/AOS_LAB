#include <stdio.h>
#include <stdlib.h>
#include <linux/elf.h>
#include <sys/mman.h>
#include <asm/auxvec.h>
#include <fcntl.h>
#include <signal.h>
#include <ucontext.h>

#define SIZE 1048576*128
#define PATH_MAX 1024
Elf64_Ehdr *hdr;
Elf64_Shdr *shdr;
Elf64_Phdr *phdr;
char *mem;
char *filePath;
char *atphdr;
int fd;

#define AT_NULL   0 /* end of vector */
#define AT_IGNORE 1 /* entry should be ignored */
#define AT_EXECFD 2 /* file descriptor of program */
#define AT_PHDR   3 /* program headers for program */
#define AT_PHENT  4 /* size of program header entry */
#define AT_PHNUM  5 /* number of program headers */
#define AT_PAGESZ 6 /* system page size */
#define AT_BASE   7 /* base address of interpreter */
#define AT_FLAGS  8 /* flags */
#define AT_ENTRY  9 /* entry point of program */
#define AT_NOTELF 10    /* program is not ELF */
#define AT_UID    11    /* real uid */
#define AT_EUID   12    /* effective uid */
#define AT_GID    13    /* real gid */
#define AT_EGID   14    /* effective gid */
#define AT_PLATFORM 15  /* string identifying CPU for optimizations */
#define AT_HWCAP  16    /* arch dependent hints at CPU capabilities */
#define AT_CLKTCK 17    /* frequency at which times() increments */
/* AT_* values 18 through 22 are reserved */
#define AT_SECURE 23   /* secure mode boolean */
#define AT_BASE_PLATFORM 24 /* string identifying real platform, may
                 * differ from AT_PLATFORM. */
#define AT_RANDOM 25    /* address of 16 random bytes */

#define AT_EXECFN  31   /* filename of program */


#define PAGE_START(x, a) ((x) & ~((a)-1))
#define PAGE_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define PAGE_OFFSET(x, a) (x & (a - 1))
#define PAGE_SIZE 4096

void segv_handler(int sig,siginfo_t *si, void *unused){
    //printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    if(si->si_addr == NULL){
        exit(-1);
    }
    //printf("Implements the handler only\n");
    long addr = si->si_addr;
    /*
    if(addr == 0x830440){
        int tmp = 0;
    }
    */
    int prot = 0;
    for(int i=0;i<hdr->e_phnum;i++){
        if(phdr[i].p_type != PT_LOAD) continue;
        
        if (addr < phdr[i].p_vaddr || addr >= phdr[i].p_vaddr+phdr[i].p_memsz) 
            continue;
        
        u_int64_t size = phdr[i].p_filesz + PAGE_OFFSET(phdr[i].p_vaddr,PAGE_SIZE);
        u_int64_t off = phdr[i].p_offset - PAGE_OFFSET(phdr[i].p_vaddr,PAGE_SIZE);
        size = PAGE_ALIGN(size,PAGE_SIZE);
        if(size==0) {
            continue;
        }
        u_int64_t mapadr = PAGE_START(phdr[i].p_vaddr,PAGE_SIZE);
        u_int64_t addrpage = PAGE_START(addr,PAGE_SIZE);
        int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
        if(mmap(addrpage,PAGE_SIZE,PROT_WRITE|PROT_READ,flags,-1,0)==-1){
            //printf("MMAP WRONG in segvhandler\n");  
            exit(-1);
        }
        

         
        if(phdr[i].p_flags&PF_W){
             prot = prot|PROT_WRITE;
        }
        prot = prot|PROT_READ;
        
        if(phdr[i].p_flags & PF_X == 1){
            prot = prot|PROT_EXEC;
        }
        
        int64_t offset = addrpage - mapadr;

        if(offset<phdr[i].p_filesz){
            long cpy;
            if(offset + PAGE_SIZE> phdr[i].p_filesz){
                cpy = phdr[i].p_filesz - offset;
            }else{
                cpy = PAGE_SIZE;
            }
            pread(fd,addrpage,cpy,offset+off);
        }

        //printf("size %lx off %lx on MAP %lx with %d permision\n",size,off,mapadr,prot);
        mprotect(addrpage,PAGE_SIZE,prot);
        return 0;
    }

    return -1;
}

void new_aux_ent(unsigned long long ** ptr, unsigned long long val, unsigned long long id){
    *(--(*ptr))=val;
    *(--(*ptr))=id;
}

void* build_stack(int argc, char** argv, char** envp){
    int* source = (int*) argc;
    int size = 1024*1024*128;
    char *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
    addr = addr + 512*1024*128;
    memset(addr,0,size);
    *(int*)addr = argc-1;
    char *retaddr = addr;
    addr = addr+8;

    memcpy(addr,(argv+2),sizeof(char**) * (argc-2)); // TODO Last one should be changed
    *(char**)addr = argv[1];

    addr = addr + (sizeof (char**))*(argc-1);

    addr = addr + 8;
    for(;*envp!=0;envp++){
        *(char**)addr = (*envp);
        addr = addr+8;
    }
    addr = addr+8;

    envp ++; // From here, it's auxiluary vecotr;
    for(;*envp !=0;envp = envp +2){
        switch((long)(*envp)){
        case AT_ENTRY:
            *(char**)addr = *envp;
            addr += 8;
            *(u_int64_t**)addr = hdr->e_entry;
            addr += 8;
        break;
        case AT_PHENT:
            *(char**)addr = *envp;
            addr += 8;
            *(char**)addr = hdr->e_phentsize;
            addr += 8;
        break;
        case AT_PHDR:
            *(char**)addr = *envp;
            addr += 8;
            *(u_int64_t**)addr = atphdr;
            addr += 8;
        break;
        case AT_PHNUM:
            *(char**)addr = *envp;
            addr += 8;
            *(u_int64_t**)addr = hdr->e_phnum;
            addr += 8;
        break;
        case AT_BASE:
            *(char**)addr = *envp;
            addr += 8;
            *(char**)addr = 0;
            addr += 8;
        break;
        case AT_EXECFN:
            *(char**)addr = *envp;
            addr += 8;
            //*(char**)addr = filePath;// exe file dir
            *(char**)addr = mmap(0,1024,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);// exe file dir
            strcpy(*(char**)addr,filePath);
            addr += 8;
        break;
        case AT_RANDOM:
            *(char**)addr = *envp;
            addr += 8;
            *(char**)addr = mmap(0,32,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);// exe file dir
            **(long**)addr = 0x4861210;
            addr += 8;
        break;
        case AT_PLATFORM:
            *(char**)addr = *envp;
            addr += 8;
            *(char**)addr = mmap(0,64,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);// exe file dir
            strcpy(*(char**)addr,*(envp+1));
            addr += 8;
        break;
        default:
            *(char**)addr = *envp;
            addr += 8;
            *(char**)addr = *(envp+1);
            addr += 8;
        break;     
        }
    }

    addr = addr+8;
    return (void*) retaddr;


}

int main(int argc, char** argv, char** envp)
{
    char* buf = mmap(0,SIZE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    mem = mmap(0,SIZE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    FILE* elf = fopen(argv[1], "rb");
    if(elf==0){
        printf("Can't open elf file");
        exit(-1);
    }
    fread(buf, SIZE, 1, elf);
    hdr = buf;
    shdr = buf+hdr->e_shoff;
    phdr = buf+hdr->e_phoff;
   
   
    struct sigaction sa;
    sa.sa_flags =  SA_ONSTACK | SA_SIGINFO;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask); 
    if(sigaction(SIGSEGV,&sa,NULL)==-1){
        printf("SIGHANDLE setup goes wrong\n");
    }
     
    fd = open(argv[1],O_RDONLY);
    if(fd==0){
        printf("Can't open elf file");
        exit(-1);
    }

    //filePath = mmap(0,PATH_MAX,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    //strcpy(filePath,argv[0]);
    filePath = argv[0];
    char * tmp = strrchr(argv[0],'/');
    tmp = tmp+1;
    strcpy(tmp,argv[1]);

    if(mem==0){
        printf("memory for elf fail");
        exit(-1);
    }
    int (*entry)(void);
    
    for (int i=0;i<hdr->e_phnum;i++){
        int prot=0;
        if(phdr[i].p_type != PT_LOAD) continue;
        if(phdr[i].p_flags&PF_W){
             prot = prot|PROT_WRITE;
        }
        
        prot = prot|PROT_READ;
        
        if(phdr[i].p_flags & PF_X == 1){
            prot = prot|PROT_EXEC;
        }
        u_int64_t size = phdr[i].p_filesz + PAGE_OFFSET(phdr[i].p_vaddr,PAGE_SIZE);
        u_int64_t off = phdr[i].p_offset - PAGE_OFFSET(phdr[i].p_vaddr,PAGE_SIZE);
        size = PAGE_ALIGN(size,PAGE_SIZE);
        if(size==0) {
            continue;
        }
        u_int64_t mapadr = PAGE_START(phdr[i].p_vaddr,PAGE_SIZE);
        printf("size %lx off %lx on MAP %lx with %d permision\n",size,off,mapadr,prot);
        int diff = phdr[i].p_memsz-phdr[i].p_filesz;
        if(diff>0){
            diff= PAGE_ALIGN(diff,PAGE_SIZE);
            printf("Additional size %lx off %lx on MAP %lx with %d permision\n",diff,0,mapadr+size,prot);
        }
        //memcpy(mapadr,off+buf,size);
        //mprotect(mapadr,size,prot);
    }
   
  /*
    for (int i=0;i<hdr->e_phnum;i++){
        int prot=0;
        if(phdr[i].p_type & PT_LOAD ==0) continue;
        if(phdr[i].p_flags&PF_W ==1){
             prot = prot|PROT_WRITE;
        }else{
            prot = prot|PROT_READ;
        }
        if(phdr[i].p_flags & PF_X == 1){
            prot = prot|PROT_EXEC;
        }
        u_int64_t size = phdr[i].p_memsz;
        u_int64_t off = phdr[i].p_offset;
        u_int64_t mapadr = phdr[i].p_vaddr;
        char* checker;
        if(size ==0) continue;
        checker = mmap(mapadr,size,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        if (checker ==-1){
            printf("MAPPING has error\n");
        }
        memcpy(mapadr,buf+off,size);
        printf("size %d off %d on MAP %p with %d permision\n",size,off,checker,prot);
    }
    */
    entry = hdr->e_entry; // HARD CODED
    //mprotect(mem,SIZE,PROT_READ|PROT_WRITE|PROT_EXEC);
    void* stack = build_stack(argc,argv,envp);
    
    asm("movq $0, %rdx;"
    "movq $0, %rbx;"
    "movq $0, %rax;"
    "movq $0, %rcx;"
    "movq $0, %rsi;"
    "movq $0, %rdi;"
    "movq $0, %r8;"
    "movq $0, %r9;"
    "movq $0, %r10;"
    "movq $0, %r11;"
    "movq $0, %r12;"
    "movq $0, %r13;"
    "movq $0, %r14;"
    "movq $0, %r15;"
    );
    
    asm("movq %0, %%rsp\n\t" : "+r" (stack));
	asm("movq %0, %%rax\n\t" : "+r" (entry));
	asm("jmp *%rax\n\t");	
}
