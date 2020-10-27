#include <stdio.h>
#include <stdlib.h>
#include <linux/elf.h>
#include <sys/mman.h>

#define SIZE 1048576
char buf[SIZE];

int main(int argc, char** argv)
{
    FILE* elf = fopen(argv[1], "rb");
    if(elf==0){
        printf("Can't open elf file");
        exit(-1);
    }
    fread(buf, sizeof buf, 1, elf);
    Elf64_Ehdr *hdr = buf;
    Elf64_Shdr *shdr = buf+hdr->e_shoff;
    Elf64_Phdr *phdr = buf+hdr->e_phoff;
    
    char* mem = mmap(0,SIZE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(mem==0){
        printf("memory for elf fail");
        exit(-1);
    }
    memset(mem,0,SIZE);

    for(int i=0;i<hdr->e_shnum;i++){
        int prot=0;
        if(shdr[i].sh_flags & SHF_WRITE){
            prot = prot|PROT_WRITE;
        }
        else
        {
            prot = prot|PROT_READ;
        }
        if(shdr[i].sh_flags & SHF_EXECINSTR){
            prot = prot|PROT_EXEC;
        }
        mprotect(buf+shdr[i].sh_offset,shdr[i].sh_size,prot);
        printf("%d\n",shdr[i].sh_type);
    }




        
}
