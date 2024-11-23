#include <stdio.h>

int main() {
  double x = 0;

  for (int n = 0; n < 1000000000000; n++) {
    double z = 1.0 / (2 * n + 1);
    if (n % 2 == 1) {
      z *= -1;
    }
    x = (x + z);
  }

  double pi = 4 * x;
  printf("The value of pi is: %f\n", pi);
  return 0;
}