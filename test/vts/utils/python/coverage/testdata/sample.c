#include <stdlib.h>
#include <stdio.h>


int main(argc, argv)
int argc;
char **argv;

{
  int x, y;
  x = 100;
  y = 250;
  testFunctionName(x + y);

  printf("The value of x is %d. \n", x);

  if (x > y) {
    printf("X is bigger.");
  } else {
    printf("Y is bigger.");
  }

  int z = 0;
  int i = 0;
  for (; i < y; i++) {
    z = -z + 2*i;
  }

  printf("The value of z is %d.\n", z);

  exit(0);

}

int testFunctionName(a)
int a;

{
  int b;
  b = a << 2;
  printf("%i times 4 = %i\n", a, b);
  return b;
}