#ifndef _BLOOM_FILTER_
#define _BLOOM_FILTER_
#include <math.h>
#include <bitset>
#include <fstream>
#include "MurmurHash3.h"
#include <iostream>
#include <string>
#include <stdlib.h> /* strtoull */
#include <services/hash.h>
#include <chrono>
using namespace std::chrono;

/*
* Optimal num bits / element given by:
* -1.44 * log2(p) * n
* p is probability of false positives, n is number of elements
* using p = 1e-6 (1 in a million)
* with n = 551509767 (number of passwords)
* number of bits = 15829134822
*/

#define NUM_BITS 15829134822
#define NUM_HASH_FNS 20

#include <services/large_int.h>

using namespace std;

class BloomFilter
{
private:
  bitset<NUM_BITS> bit_array;
  void calculate_hashes(string element, LargeInt *murmur_output, LargeInt *sha256_output);

public:
  BloomFilter();
  BloomFilter(string s);
  void add_element(string element);
  bool check_element(string element);
  void save_to_file(string filename);
  void init_from_textfile(ifstream &inFile);
  void init_from_dbfile(ifstream &dbfile);
};
#endif