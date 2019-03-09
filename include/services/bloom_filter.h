#ifndef _BLOOM_FILTER_
#define _BLOOM_FILTER_
#include "MurmurHash3.h"
#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>
#include <services/hash.h>
#include <stdlib.h> /* strtoull */
#include <string>
#include <thread>
#include <gmp.h>

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
#define NUM_THREADS 8

using namespace std;

class BloomFilter
{
private:
  bitset<NUM_BITS> bit_array;
  thread *threads;
  mutex file_mutex;
  mutex bitset_mutex;
  void calculate_hashes(string element, mpz_t &murmur_output,
                        mpz_t &sha256_output);
  void calculate_indices(string element, uint64_t *indices);
  void insert_passwords_thread(ifstream &inFile);
  mpz_t large_prime;
  void add_string_to_buffer();

public:
  BloomFilter();
  ~BloomFilter();
  void add_element(string element);
  bool check_element(string element);
  void save_to_file(string filename);
  void init_from_textfile(ifstream &inFile);
  void init_from_dbfile(ifstream &dbfile);
};
#endif
