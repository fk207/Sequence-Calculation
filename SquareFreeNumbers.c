/* File: factorize.c */

/* Factorizes all numbers in a range */

#include <stdio.h>

int returnfactors(int tempn, int factor);
int findnextprime(int cp);
int sqfcounter = 1; //1 is considered square free, but isnt counted in the loop

int main(){

  int min = 1;
  int max = 111111111;

  
  for(int num = min; num <= max; num++){
    int currentprime = 2;
    int temp = num;
    //printf("Prime factors of %d: ", num);
    while(temp > 1){
        temp = returnfactors (temp, currentprime);
        if(temp == -1){ 
            break;
        }
        else if(temp == 1){
            sqfcounter++;
            //printf("%d ", num);
            break;
        }
        currentprime = findnextprime(currentprime);
        
      
    }
    //printf("\n");
  }
    printf("Square free numbers between %d and %d: %d\n", min, max, sqfcounter);
    
  return 0;
}



int returnfactors(int tempn, int factor){
  
  int counter =0;
  while(tempn % factor == 0){
        tempn = tempn / factor;
        counter++;
    }
    if(counter > 1){
        return -1;
    }
    return tempn;
}

int findnextprime(int cp){
  int np = cp + 2;
  int j = cp;
  while(1){
        
        int count = 0;
        if(cp == 2){
            return 3;
        }
        for(int i = 3; i*i <=j;i++){
            if(j % i == 0){
                count++;
                break;
            }
        } 
        if(np != cp){
          return np;
        }
        else{
          np = np + 2;
        }

    }

}