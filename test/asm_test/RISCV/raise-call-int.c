#include <stdio.h>

int increment(int X) {
   return X + 1;
}

int main(void) {
   int X = 3;
   printf("%d\n", increment(X));
   return 0; 
}
