#ifndef __GOLOMB_COMBING__
#define __GOLOMB_COMBING__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <cstring>

using namespace std;

// with a false positive rate of 1 in 1 billion
// m is ~ 693M, b is 30, cutoff is ~380M
// with a false positive rate of 1 in 1 million
// m is ~693k, b is 20, cutoff is ~355k
// with a false positive rate of 1 in 1000
// m is ~693, b is 10, cutoff is ~331

template <class Tuint>
class GolombCoding
{
private:
  Tuint m;      // based on false positive rate
  Tuint cutoff; // based on m and b

  Tuint n_decompressed; // number of bits when decompressed

  void encode_unary(Tuint num_bytes, uint8_t remainder, uint8_t *unary_bytes);
  void encode_truncated_binary(Tuint *remainder, uint8_t *remainder_bytes, uint8_t *num_bits);
  Tuint encode_ones(Tuint starting_bit, Tuint num_ones);
  Tuint decode_unary(Tuint bit_index, Tuint *quotient);
  Tuint decode_truncated_binary(Tuint bit_index, Tuint *truncated_binary);
  Tuint decode_ones(Tuint starting_bit, Tuint *num_ones);

  Tuint shift_remaining_bytes(Tuint starting_bit, Tuint num_to_shift);
  Tuint encode_quotient_and_remainder(Tuint starting_bit, Tuint quotient, Tuint remainder);
  Tuint encode_quotient_and_remainder(Tuint starting_bit, Tuint number);
  Tuint encode_quotient(Tuint starting_bit, Tuint quotient);
  Tuint encode_remainder(Tuint starting_bit, Tuint remainder);

public:
  uint8_t *bytes = NULL;
  Tuint n_bits = 0; // number of bits when compressed
  Tuint b;          // based on m

  void init(Tuint _m, Tuint _size, vector<pair<Tuint, bool>> const &encodings);
  void init(Tuint _m, Tuint _size);
  void realloc_bits(Tuint new_length);
  void add_bit(Tuint position);
  bool check_bit(Tuint position);
};

#include "../templates/golomb_coding.tpp"

#endif
