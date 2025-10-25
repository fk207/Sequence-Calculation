#include <stdio.h>

int main(){
    int max;
    float sum = 0;
    double negpos = 1.0;
    scanf("%d",&max);
    for(int i=2;i<=max;i+2){
        sum += negpos/(i*(i+1)*(i+2));
        negpos *= -1;
    }
    printf("%f",sum);

    return 0;
}