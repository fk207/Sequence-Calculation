#include <stdio.h>

int main(){
    int max;
    float sum = 1;
    float negpos = -1.0;
    scanf("%d",&max);
    for(int i=2;i<=max;i++){
        sum += negpos/(i*i);
        negpos *= -1;
    }
    printf("%f",sum);

    return 0;
}