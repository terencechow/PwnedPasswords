#ifndef _LARGE_INT_
#define _LARGE_INT_
#include <stdlib.h>

// 256 bit large int
class LargeInt
{
private:
  uint64_t hi(uint64_t x);
  uint64_t lo(uint64_t x);
  uint64_t multmod(uint64_t a, uint64_t b, uint64_t n);
  uint64_t addmod(uint64_t a, uint64_t b, uint64_t n);

public:
  uint64_t bits[4];
  LargeInt();
  LargeInt(uint64_t *u64_bits, uint8_t len);
  LargeInt(unsigned char *bytes, uint8_t len);
  LargeInt operator+(const LargeInt other);
  LargeInt operator*(uint8_t multicand);
  uint64_t operator%(uint64_t divisor);
};
#endif