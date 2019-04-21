#define XXH_INLINE_ALL
#include "xxhash.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdlib.h> /* strtoull */
#include <string>
#include <thread>
#include <gmp.h>
#include <services/golomb_coding.h>
#include <services/hash.h>
#include <string>

using namespace std::chrono;
using namespace std;

template <class Tuint>
class GolombSet
{
private:
  uint64_t golomb_set_size;
  uint64_t section_size;
  uint64_t num_sections;
  uint64_t num_passwords;

  // roughly 1 in 1.1M, M & b are optimized when m = 1.497137 * 2^b https://gist.github.com/sipa/576d5f09c3b86c3b1b75598d799fc845
  double false_positive_rate = 0.000000883067;
  GolombCoding<Tuint> *codings;

  uint64_t get_index(string element);
  void get_section_and_position(string element, uint64_t *section, Tuint *position);
  void insert_passwords_thread(ifstream &inFile, uint8_t thread_index, uint64_t *temp_bytes);
  uint64_t get_num_passwords(ifstream &inFile);
  Tuint get_m_param();
  mutex file_mutex;
  uint64_t processed;
  string hash_plaintext(string element);

public:
  GolombSet(double fpr) : false_positive_rate(fpr){};
  ~GolombSet();
  void add_password(string element);
  void add_hash(string element);
  bool check_password(string element);
  bool check_hash(string element);
  void save_to_file(string filename);
  void init_from_textfile(string password_filename, string out_filename);
  void init_from_dbfile(string infileName);
};

#include "../templates/golomb_set.tpp"