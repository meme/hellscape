#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

uint32_t target(uint32_t n) {
  uint32_t mod = n % 4;
  uint32_t result = 0;

  if (mod == 0) {
    result = (n | 0xBAAAD0BF) * (2 ^ n);
  } else if (mod == 1) {
    result = (n & 0xBAAAD0BF) * (3 + n);
  } else if (mod == 2) {
    result = (n ^ 0xBAAAD0BF) * (4 | n);
  } else {
    result = (n + 0xBAAAD0BF) * (5 & n);
  }

  return result;
}

int main() {
  printf("%d\n", target(10));
  return 0;
}