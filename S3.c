#include <stdio.h>

int main(){
    int max;
    double sum = 1;
    double negpos = -1.0;
    scanf("%d",&max);
    for(int i=3;i<=max;i+2){
        sum += negpos/i;
        negpos *= -1;
    }
    printf("%f",sum);

    return 0;
}