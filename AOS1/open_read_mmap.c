#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

int global=0;
void foo(){
    char a;
    a='a';
    return;
}
int main()
{
    char ch;
    FILE *fp;
    struct rusage resource;

    fp = fopen("/proc/self/maps", "r"); // read mode

    if (fp == NULL)
    {
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }
    printf("Address of code : %p\n",&foo);
    printf("Address of data : %p\n",&ch);
    printf("Address of global data: %p\n",&global);

    while((ch = fgetc(fp)) != EOF)
        printf("%c", ch);

    fclose(fp);
    if (getrusage(RUSAGE_SELF,&resource) <0){
    }
    printf("utime_sec : %ld \n",resource.ru_utime.tv_sec);
    printf("utime_usec : %ld \n",resource.ru_utime.tv_usec);
    printf("stime_sec : %ld \n",resource.ru_stime.tv_sec);
    printf("stime_usec : %ld \n",resource.ru_stime.tv_usec);
    printf("maxrss : %ld \n",resource.ru_maxrss);
    printf("minflt : %ld \n",resource.ru_minflt);
    printf("majflt : %ld \n",resource.ru_majflt);
    printf("inblock : %ld \n",resource.ru_inblock);
    printf("oublock : %ld \n",resource.ru_oublock);


    return 0;
}

