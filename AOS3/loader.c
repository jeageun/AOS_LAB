#include <stdio.h>
#include <stdlib.h>
#include <linux/elf.h>
#include <sys/mman.h>

#define SIZE 1048576*128

int main(int argc, char** argv)
{
    char* mem = mmap(0,SIZE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    FILE* elf = fopen(argv[1], "rb");
    if(elf==0){
        printf("Can't open elf file");
        exit(-1);
    }
    fread(mem, SIZE, 1, elf);
    Elf64_Ehdr *hdr = mem;
    Elf64_Shdr *shdr = mem+hdr->e_shoff;
    Elf64_Phdr *phdr = mem+hdr->e_phoff;
    
    if(mem==0){
        printf("memory for elf fail");
        exit(-1);
    }
    
    /*
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
        mprotect(mem+shdr[i].sh_offset,shdr[i].sh_size,prot);
        printf("%d\n",shdr[i].sh_type);
    }
    */
    int (*entry)(void);
    
    for (int i=0;i<hdr->e_phnum;i++){
        int prot=0;
        if(phdr[i].p_type != PT_LOAD) continue;
        if(phdr[i].p_flags==PF_W){
             prot = prot|PROT_WRITE;
        }else{
            prot = prot|PROT_READ;
        }
        if(phdr[i].p_flags == PF_X){
            prot = prot|PROT_EXEC;
        }
        memcpy(mem+phdr[i].p_vaddr,mem+phdr[i].p_offset,phdr[i].p_memsz);
        mprotect(mem+phdr[i].p_offset,phdr[i].p_memsz,prot);
    }
   
    entry = mem+shdr[1].sh_offset; // HARD CODED
    mprotect(mem,SIZE,PROT_READ|PROT_WRITE|PROT_EXEC);
    goto *entry;
}
