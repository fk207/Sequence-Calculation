#include <stdio.h>

int main(){
    int max;
    double sum = 1;
    scanf("%d",&max);
    for(int i=2;i<=max;i++){
        sum += 1.0/(i*i*i*i);
    }
    printf("%f",sum);

    return 0;
}