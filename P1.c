#include <stdio.h>

int main(){
    int max;
    float gen = 1;
    scanf("%d",&max);
    for(int i=2;i<=max;i+2){
        gen *= (i/(i-1))*(i/(i+1));
    }
    printf("%f",gen);

    return 0;
}