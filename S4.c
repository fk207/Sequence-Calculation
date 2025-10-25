#include <stdio.h>

int main(){
    int max;
    float sum = 0;
    scanf("%d",&max);
    for(int i=1;i<=max;i+2){
        sum += 1.0/(i*(i+2));
    }
    printf("%f",sum);

    return 0;
}