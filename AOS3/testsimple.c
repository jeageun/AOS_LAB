//#include <stdio.h>
char tmp[65536];
int main(){
    int arr[10];
    int arrb[10];
    for (int i=0;i<10;i++){
        arr[i] = i;
    }
    for (int j=0;j<10;j++){
        arrb[j] = arr[j]*10;
//        printf("%d\n",arrb[j]);
    }
    
}
