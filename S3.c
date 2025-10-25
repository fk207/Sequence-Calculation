#include <stdio.h>

int main(){
    int max;
    float sum = 1;
    int negpos = -1;
    scanf("%d",&max);
    for(int i=3;i<=max;i+2){
        sum += negpos * 1.0/i;
        negpos *= -1;
    }
    printf("%f",sum);

    return 0;
}