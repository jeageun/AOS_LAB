#include <stdio.h>
#include <sys/time.h>
#include <time.h>


static double gtod_ref_time_sec = 0.0;

/* Adapted from the bl2_clock() routine in the BLIS library */

double dclock()
{
        double         the_time, norm_sec;
        struct timeval tv;

        gettimeofday( &tv, NULL );

        if ( gtod_ref_time_sec == 0.0 )
                gtod_ref_time_sec = ( double ) tv.tv_sec;

        norm_sec = ( double ) tv.tv_sec - gtod_ref_time_sec;

        the_time = norm_sec + tv.tv_usec * 1.0e-6;

        return the_time;
}


#define SIZE 1024
char tmp[65536];
int arrc[1024*1024];
int main(){
    double dtime;
    dtime = dclock();
    int arr[SIZE];
    int arrb[SIZE];
    
    for (int i=0;i<SIZE;i++){
        arr[i] = i;
    }/*
    for (int j=0;j<SIZE;j++){
        arrb[j] = arr[j]*10;
    }
    */
    /*
    for(int j=0;j<;j++){
        arrc[j] = arrb[j%SIZE];
    }
    */
    dtime = dclock()-dtime;
    printf("%f time\n",dtime);
    
}
