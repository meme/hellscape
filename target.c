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

__attribute__((obfus("nobcf"))) __attribute__((obfus("fla"))) void make_kn(unsigned char *k1, const unsigned char *l, int bl)
{
    int i;
    unsigned char c = l[0], carry = c >> 7, cnext;
    for (i = 0; i < bl - 1; i++, c = cnext)
        k1[i] = (c << 1) | ((cnext = l[i + 1]) >> 7);

    k1[i] = (c << 1) ^ ((0 - carry) & (bl == 16 ? 0x87 : 0x1b));
}

int main() {
  printf("%d\n", target(10));
  return 0;
}