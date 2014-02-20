#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int factorial(int);
 
int main(int argc, char**argv)
{

  char *input;
  int fact;
  int n = 1;
  float f=0.0;
  int input_check;
  
  input = argv[1];
  input_check = strlen(input);
  n = atoi(input);
  f = atof(input);
  
  if ((n < 1)||((f-n)>0)||(input_check>2)){ 
    printf("Huh?\n");
  }
  else if(n>12){
  	printf("Overflow\n");
  }
  else{
    fact = factorial(n);
    printf("%d\n", fact );
  }
  return 0;
}
 int factorial(int n)
{
  if (n == 0)
    return 1;
else
    return(n * factorial(n-1));
}
