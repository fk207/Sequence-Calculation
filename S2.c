#include <stdio.h>
#include <mpfr.h>

int main(){
    int max;
    mpfr_set_default_prec(512);
    mpfr_t sum, temp, negpos , minone,temp2,one;
    mpfr_init_set_str(sum, "1.0", 10, MPFR_RNDN); 
    mpfr_init_set_str(negpos, "-1.0", 10, MPFR_RNDN); 
    mpfr_init_set_str(minone, "-1.0", 10, MPFR_RNDN);
    mpfr_init_set_str(one, "1.0", 10, MPFR_RNDN);



    scanf("%d",&max);
    for(int i=2;i<=max;i++){
        mpfr_init(temp);
        mpfr_init(temp2);
        mpfr_init_set_d(temp2,i,MPFR_RNDN);
        mpfr_div(temp,one,temp2,MPFR_RNDN);
        mpfr_mul(temp,temp,negpos,MPFR_RNDN);
        mpfr_add(sum,sum,temp,MPFR_RNDN);
        mpfr_mul(negpos,negpos,minone,MPFR_RNDN);
        
    }
    mpfr_printf("%.100Rf",sum);

    return 0;
}